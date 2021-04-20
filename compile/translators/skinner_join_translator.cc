#include "compile/translators/skinner_join_translator.h"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_set.h"
#include "compile/ir_registry.h"
#include "compile/proxy/function.h"
#include "compile/proxy/loop.h"
#include "compile/proxy/printer.h"
#include "compile/proxy/struct.h"
#include "compile/proxy/value.h"
#include "compile/proxy/vector.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "compile/translators/scan_translator.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/expression/binary_arithmetic_expression.h"
#include "plan/expression/case_expression.h"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/conversion_expression.h"
#include "plan/expression/expression_visitor.h"
#include "plan/expression/literal_expression.h"
#include "plan/expression/virtual_column_ref_expression.h"
#include "util/vector_util.h"

namespace kush::compile {

class PredicateColumnCollector : public plan::ImmutableExpressionVisitor {
 public:
  PredicateColumnCollector() = default;
  virtual ~PredicateColumnCollector() = default;

  std::vector<std::reference_wrapper<const plan::ColumnRefExpression>>
  PredicateColumns() {
    // dedupe, should actually be doing this via hashing
    absl::flat_hash_set<std::pair<int, int>> exists;

    std::vector<std::reference_wrapper<const plan::ColumnRefExpression>> output;
    for (auto& col_ref : predicate_columns_) {
      std::pair<int, int> key = {col_ref.get().GetChildIdx(),
                                 col_ref.get().GetColumnIdx()};
      if (exists.contains(key)) {
        continue;
      }

      output.push_back(col_ref);
      exists.insert(key);
    }
    return output;
  }

  void Visit(const plan::ColumnRefExpression& col_ref) override {
    predicate_columns_.push_back(col_ref);
  }

  void VisitChildren(const plan::Expression& expr) {
    for (auto& child : expr.Children()) {
      child.get().Accept(*this);
    }
  }

  void Visit(const plan::BinaryArithmeticExpression& arith) override {
    VisitChildren(arith);
  }

  void Visit(const plan::CaseExpression& case_expr) override {
    VisitChildren(case_expr);
  }

  void Visit(const plan::IntToFloatConversionExpression& conv) override {
    VisitChildren(conv);
  }

  void Visit(const plan::LiteralExpression& literal) override {}

  void Visit(const plan::AggregateExpression& agg) override {
    throw std::runtime_error("Aggregates cannot appear in join predicates.");
  }

  void Visit(const plan::VirtualColumnRefExpression& virtual_col_ref) override {
    throw std::runtime_error(
        "Virtual column references cannot appear in join predicates.");
  }

 private:
  std::vector<std::reference_wrapper<const plan::ColumnRefExpression>>
      predicate_columns_;
};

template <typename T>
class TupleIdxHandler {
 public:
  TupleIdxHandler(
      ProgramBuilder<T>& program,
      std::function<void(typename ProgramBuilder<T>::Value& v)> body) {
    auto& current_block = program.CurrentBlock();

    func = &program.CreateFunction(program.VoidType(),
                                   {program.PointerType(program.I32Type())});
    auto args = program.GetFunctionArguments(*func);

    auto& idx_ptr =
        program.PointerCast(args[0], program.PointerType(program.I32Type()));

    body(idx_ptr);

    program.SetCurrentBlock(current_block);
  }

  typename ProgramBuilder<T>::Function& Get() { return *func; }

 private:
  typename ProgramBuilder<T>::Function* func;
};

template <typename T>
SkinnerJoinTranslator<T>::SkinnerJoinTranslator(
    const plan::SkinnerJoinOperator& join, ProgramBuilder<T>& program,
    std::vector<std::unique_ptr<OperatorTranslator<T>>> children)
    : OperatorTranslator<T>(join, std::move(children)),
      join_(join),
      program_(program),
      expr_translator_(program_, *this) {}

template <typename T>
void SkinnerJoinTranslator<T>::Produce() {
  auto child_translators = this->Children();
  auto child_operators = this->join_.Children();
  auto conditions = join_.Conditions();

  std::vector<absl::btree_set<int>> predicates_per_table(
      child_operators.size());
  std::vector<absl::flat_hash_set<int>> tables_per_predicate(conditions.size());
  PredicateColumnCollector total_collector;
  for (int i = 0; i < conditions.size(); i++) {
    auto& condition = conditions[i];

    condition.get().Accept(total_collector);

    PredicateColumnCollector collector;
    condition.get().Accept(collector);
    auto predicate_columns = collector.PredicateColumns();
    for (const auto& col_ref : predicate_columns) {
      predicates_per_table[col_ref.get().GetChildIdx()].insert(i);
      tables_per_predicate[i].insert(col_ref.get().GetChildIdx());
    }
  }
  predicate_columns_ = total_collector.PredicateColumns();

  // 1. Materialize each child.
  std::vector<std::unique_ptr<proxy::StructBuilder<T>>> structs;
  for (int i = 0; i < child_translators.size(); i++) {
    auto& child_translator = child_translators[i].get();
    auto& child_operator = child_operators[i].get();

    // Create struct for materialization
    structs.push_back(std::make_unique<proxy::StructBuilder<T>>(program_));
    const auto& child_schema = child_operator.Schema().Columns();
    for (const auto& col : child_schema) {
      structs.back()->Add(col.Expr().Type());
    }
    structs.back()->Build();

    // Init vector of structs
    buffers_.push_back(
        std::make_unique<proxy::Vector<T>>(program_, *structs.back(), true));

    indexes_.push_back({});

    // For each predicate column on this table, declare an index.
    for (auto& predicate_column : predicate_columns_) {
      if (i != predicate_column.get().GetChildIdx()) {
        continue;
      }

      switch (predicate_column.get().Type()) {
        case catalog::SqlType::SMALLINT:
          indexes_.back().push_back(
              std::make_unique<
                  proxy::ColumnIndexImpl<T, catalog::SqlType::SMALLINT>>(
                  program_, true));
          break;
        case catalog::SqlType::INT:
          indexes_.back().push_back(
              std::make_unique<
                  proxy::ColumnIndexImpl<T, catalog::SqlType::INT>>(program_,
                                                                    true));
          break;
        case catalog::SqlType::BIGINT:
          indexes_.back().push_back(
              std::make_unique<
                  proxy::ColumnIndexImpl<T, catalog::SqlType::BIGINT>>(program_,
                                                                       true));
          break;
        case catalog::SqlType::REAL:
          indexes_.back().push_back(
              std::make_unique<
                  proxy::ColumnIndexImpl<T, catalog::SqlType::REAL>>(program_,
                                                                     true));
          break;
        case catalog::SqlType::DATE:
          indexes_.back().push_back(
              std::make_unique<
                  proxy::ColumnIndexImpl<T, catalog::SqlType::DATE>>(program_,
                                                                     true));
          break;
        case catalog::SqlType::TEXT:
          indexes_.back().push_back(
              std::make_unique<
                  proxy::ColumnIndexImpl<T, catalog::SqlType::TEXT>>(program_,
                                                                     true));
          break;
        case catalog::SqlType::BOOLEAN:
          indexes_.back().push_back(
              std::make_unique<
                  proxy::ColumnIndexImpl<T, catalog::SqlType::BOOLEAN>>(
                  program_, true));
          break;
      }
    }

    // Fill buffer/indexes
    child_idx_ = i;
    child_translator.Produce();
  }

  // 2. Setup join evaluation
  // Setup struct of predicate columns
  auto predicate_struct = std::make_unique<proxy::StructBuilder<T>>(program_);
  for (auto& predicate_column : predicate_columns_) {
    const auto& child = child_operators[predicate_column.get().GetChildIdx()];
    const auto& col =
        child.get().Schema().Columns()[predicate_column.get().GetColumnIdx()];
    auto type = col.Expr().Type();
    predicate_struct->Add(type);
  }
  predicate_struct->Build();

  proxy::Struct<T> global_predicate_struct(
      program_, *predicate_struct,
      program_.GlobalStruct(false, predicate_struct->Type(),
                            predicate_struct->DefaultValues()));

  // Setup idx array.
  std::vector<std::reference_wrapper<typename ProgramBuilder<T>::Value>>
      initial_idx_values(child_operators.size(), program_.ConstI32(0));
  auto& idx_type = program_.I32Type();
  auto& idx_array_type = program_.ArrayType(idx_type, child_operators.size());
  auto& idx_array =
      program_.GlobalArray(false, idx_array_type, initial_idx_values);

  // Setup flag array for each table.
  int total_flags = 0;
  absl::flat_hash_map<std::pair<int, int>, int> table_predicate_to_flag_idx;
  int flag_idx = 0;
  for (int i = 0; i < predicates_per_table.size(); i++) {
    total_flags += predicates_per_table[i].size();
    for (int predicate_idx : predicates_per_table[i]) {
      table_predicate_to_flag_idx[{i, predicate_idx}] = flag_idx++;
    }
  }

  std::vector<std::reference_wrapper<typename ProgramBuilder<T>::Value>>
      initial_flag_values(total_flags, program_.ConstI1(0));
  auto& flag_type = program_.I1Type();
  auto& flag_array_type = program_.ArrayType(flag_type, total_flags);
  auto& flag_array =
      program_.GlobalArray(false, flag_array_type, initial_flag_values);

  // Setup table handlers
  auto& handler_type = program_.FunctionType(program_.VoidType(), {});
  auto& handler_pointer_type = program_.PointerType(handler_type);
  auto& handler_pointer_array_type =
      program_.ArrayType(handler_pointer_type, child_translators.size());
  std::vector<std::reference_wrapper<typename ProgramBuilder<T>::Value>>
      initial_values(child_translators.size(),
                     program_.NullPtr(handler_pointer_type));
  auto& handler_pointer_array =
      program_.GlobalArray(false, handler_pointer_array_type, initial_values);

  std::vector<proxy::VoidFunction<T>> table_functions;
  int current_buffer = 0;
  for (int table_idx = 0; table_idx < child_translators.size(); table_idx++) {
    auto& child_translator = child_translators[table_idx].get();

    table_functions.push_back(proxy::VoidFunction<T>(program_, [&]() {
      auto& handler_ptr = program_.GetElementPtr(
          handler_pointer_array_type, handler_pointer_array,
          {program_.ConstI32(0), program_.ConstI32(table_idx)});
      auto& handler = program_.Load(handler_ptr);

      {
        // Unpack the predicate struct.
        auto column_values = global_predicate_struct.Unpack();

        // Set the schema value for each predicate column to be the ones from
        // the predicate struct.
        for (int i = 0; i < column_values.size(); i++) {
          auto& column_ref = predicate_columns_[i].get();

          // Set the ColumnIdx value of the ChildIdx operator to be the
          // unpacked value.
          auto& child_translator =
              child_translators[column_ref.GetChildIdx()].get();
          child_translator.SchemaValues().SetValue(column_ref.GetColumnIdx(),
                                                   std::move(column_values[i]));
        }
      }

      // if this table has an index associated with it:

      // lastTuple = -1
      // while (lastTuple  < cardinality):
      // nextTuple = lastTuple + 1
      // for each equality predicate that is active:
      //  nextTuple = max(lastTuple, GetNextTuple(index, cardinality, lastTuple,
      //  other
      //    tuple's value))
      // if next_tuple = cardinality:
      //    # done with this table
      //    break
      // fetch next_tuple
      // store next_tuple's predicate columns in struct
      // evaluate all other active predicates on next tuple
      // move to next table in join order
      // lastTuple = nextTuple

      // Loop over tuples in buffer
      proxy::Vector<T>& buffer = *buffers_[current_buffer++];
      auto cardinality = proxy::Int32<T>(program_, buffer.Size().Get());

      proxy::IndexLoop<T>(
          program_, [&]() { return proxy::Int32<T>(program_, -1); },
          [&](proxy::Int32<T>& lastTuple) { return lastTuple < cardinality; },
          [&](proxy::Int32<T>& lastTuple, auto Continue) {
            auto nextTuple = lastTuple + proxy::Int32<T>(program_, 1);

            // TODO get nextTuple from active equality predicates

            proxy::If<T>(program_, nextTuple == cardinality,
                         [&]() { Continue(cardinality); });

            // Load the current tuple of table.
            auto current_table_values =
                buffer[proxy::Int32<T>(program_, nextTuple.Get())].Unpack();

            // Store each of this table's predicate column values into
            // the global_predicate_struct
            for (int k = 0; k < predicate_columns_.size(); k++) {
              auto& col_ref = predicate_columns_[k].get();
              if (col_ref.GetChildIdx() == table_idx) {
                global_predicate_struct.Update(
                    k, *current_table_values[col_ref.GetColumnIdx()]);

                // Additionally, update this table's values to read from
                // the unpacked tuple instead of the old loaded value from
                // global_predicate_struct.
                child_translator.SchemaValues().SetValue(
                    col_ref.GetColumnIdx(),
                    std::move(current_table_values[col_ref.GetColumnIdx()]));
              }
            }

            // Evaluate every (TODO: every non-index checked) predicate that
            // references this table's columns.
            for (int predicate_idx : predicates_per_table[table_idx]) {
              auto& flag_ptr = program_.GetElementPtr(
                  flag_array_type, flag_array,
                  {program_.ConstI32(0),
                   program_.ConstI32(table_predicate_to_flag_idx.at(
                       {table_idx, predicate_idx}))});
              proxy::Bool<T> flag(program_, program_.Load(flag_ptr));
              proxy::If<T>(program_, flag, [&]() {
                auto cond = expr_translator_.Compute(conditions[predicate_idx]);
                proxy::If<T>(program_, !static_cast<proxy::Bool<T>&>(*cond),
                             [&]() { Continue(nextTuple); });
              });
            }

            // Store idx into global idx array.
            auto& idx_ptr = program_.GetElementPtr(
                idx_array_type, idx_array,
                {program_.ConstI32(0), program_.ConstI32(table_idx)});
            program_.Store(idx_ptr, nextTuple.Get());

            // Call next table handler
            program_.Call(handler, handler_type);

            // lastTuple = nextTuple
            return nextTuple;
          });
      program_.Return();
    }));
  }

  // Setup global hash table that contains tuple idx
  auto& tuple_idx_table_type = program_.PointerType(program_.I8Type());
  auto& create_fn = program_.DeclareExternalFunction(
      "_ZN4kush4data19CreateTupleIdxTableEv", tuple_idx_table_type, {});
  auto& insert_fn = program_.DeclareExternalFunction(
      "_ZN4kush4data6InsertEPSt13unordered_setISt6vectorIiSaIiEESt4hashIS4_"
      "ESt8equal_toIS4_ESaIS4_EEPii",
      program_.VoidType(),
      {tuple_idx_table_type, program_.PointerType(program_.I32Type()),
       program_.I32Type()});
  auto& free_fn = program_.DeclareExternalFunction(
      "_ZN4kush4data4FreeEPSt13unordered_setISt6vectorIiSaIiEESt4hashIS4_"
      "ESt8equal_toIS4_ESaIS4_EE",
      program_.VoidType(), {tuple_idx_table_type});
  auto& size_fn = program_.DeclareExternalFunction(
      "_ZN4kush4data4SizeEPSt13unordered_setISt6vectorIiSaIiEESt4hashIS4_"
      "ESt8equal_toIS4_ESaIS4_EE",
      program_.I32Type(), {tuple_idx_table_type});
  auto& tuple_idx_iterator_type = program_.PointerType(program_.I8Type());
  auto& begin_fn = program_.DeclareExternalFunction(
      "_ZN4kush4data5BeginEPSt13unordered_setISt6vectorIiSaIiEESt4hashIS4_"
      "ESt8equal_toIS4_ESaIS4_EEPPNSt8__detail20_Node_const_iteratorIS4_"
      "Lb1ELb1EEE",
      program_.VoidType(), {tuple_idx_table_type, tuple_idx_iterator_type});
  auto& increment_fn = program_.DeclareExternalFunction(
      "_ZN4kush4data9IncrementEPPNSt8__detail20_Node_const_"
      "iteratorISt6vectorIiSaIiEELb1ELb1E",
      program_.VoidType(), {tuple_idx_iterator_type});
  auto& get_fn = program_.DeclareExternalFunction(
      "_ZN4kush4data3GetEPPNSt8__detail20_Node_const_"
      "iteratorISt6vectorIiSaIiEELb1ELb1EEE",
      program_.PointerType(program_.I32Type()), {tuple_idx_iterator_type});
  auto& free_it_fn = program_.DeclareExternalFunction(
      "_ZN4kush4data4FreeEPPNSt8__detail20_Node_const_"
      "iteratorISt6vectorIiSaIiEELb1ELb1EEE",
      program_.VoidType(), {tuple_idx_iterator_type});

  auto& tuple_idx_table_ptr = program_.GlobalPointer(
      false, tuple_idx_table_type, program_.NullPtr(tuple_idx_table_type));
  auto& tuple_idx_table = program_.Call(create_fn);
  program_.Store(tuple_idx_table_ptr, tuple_idx_table);

  // Setup function for each valid tuple
  proxy::VoidFunction<T> valid_tuple_handler(program_, [&]() {
    // Insert tuple idx into hash table
    auto& tuple_idx_table = program_.Load(tuple_idx_table_ptr);
    auto& tuple_idx_arr =
        program_.GetElementPtr(idx_array_type, idx_array,
                               {program_.ConstI32(0), program_.ConstI32(0)});

    auto& num_tables = program_.ConstI32(child_translators.size());
    program_.Call(insert_fn, {tuple_idx_table, tuple_idx_arr, num_tables});
    program_.Return();
  });

  // 3. Execute join
  // For now, call static order by executing 0th function
  // TODO: Setup UCT join ordering

  // Set join order
  for (int i = 0; i < child_operators.size() - 1; i++) {
    // set the ith hanlder to i+1 function
    auto& handler_ptr = program_.GetElementPtr(
        handler_pointer_array_type, handler_pointer_array,
        {program_.ConstI32(0), program_.ConstI32(i)});
    program_.Store(handler_ptr, {table_functions[i + 1].Get()});
  }
  // Set the last handler to be the valid_tuple_handler
  {
    auto& handler_ptr = program_.GetElementPtr(
        handler_pointer_array_type, handler_pointer_array,
        {program_.ConstI32(0), program_.ConstI32(child_operators.size() - 1)});
    program_.Store(handler_ptr, {valid_tuple_handler.Get()});
  }

  // Toggle predicate flags
  absl::flat_hash_set<int> available_tables;
  absl::flat_hash_set<int> executed_predicates;
  for (int table_idx = 0; table_idx < child_operators.size(); table_idx++) {
    available_tables.insert(table_idx);

    // all tables in available_tables have been joined
    // execute any non-executed predicate
    for (int predicate_idx = 0; predicate_idx < conditions.size();
         predicate_idx++) {
      if (executed_predicates.contains(predicate_idx)) continue;

      bool all_tables_available = true;
      for (int table : tables_per_predicate[predicate_idx]) {
        all_tables_available =
            all_tables_available && available_tables.contains(table);
      }
      if (!all_tables_available) continue;

      executed_predicates.insert(predicate_idx);

      auto& flag_ptr = program_.GetElementPtr(
          flag_array_type, flag_array,
          {program_.ConstI32(0),
           program_.ConstI32(
               table_predicate_to_flag_idx.at({table_idx, predicate_idx}))});
      program_.Store(flag_ptr, program_.ConstI1(1));
    }
  }

  // Execute static order by calling 0th function.
  program_.Call(table_functions[0].Get(), {});

  // Loop over tuple idx table and then output tuples from each table.
  auto& tuple_it = program_.Alloca(program_.I8Type());
  program_.Call(begin_fn, {tuple_idx_table, tuple_it});
  auto size =
      proxy::Int32<T>(program_, program_.Call(size_fn, {tuple_idx_table}));

  proxy::IndexLoop<T>(
      program_, [&]() { return proxy::Int32<T>(program_, 0); },
      [&](proxy::Int32<T>& i) { return i < size; },
      [&](proxy::Int32<T>& i, std::function<void(proxy::Int32<T>&)> Continue) {
        auto& tuple_idx_arr = program_.Call(get_fn, {tuple_it});

        int current_buffer = 0;
        for (int i = 0; i < child_translators.size(); i++) {
          auto& child_translator = child_translators[i].get();

          auto& tuple_idx_ptr = program_.GetElementPtr(
              program_.I32Type(), tuple_idx_arr, {program_.ConstI32(i)});
          auto tuple_idx =
              proxy::Int32<T>(program_, program_.Load(tuple_idx_ptr));

          // TODO: Specialize scan to get tuple_idx element
          /* if (auto scan =
          dynamic_cast<ScanTranslator<T>*>(&child_translator)) {
          } else */

          {
            proxy::Vector<T>& buffer = *buffers_[current_buffer++];
            // set the schema values of child to be the tuple_idx'th tuple of
            // current table.
            child_translator.SchemaValues().SetValues(
                buffer[tuple_idx].Unpack());
          }
        }

        // Compute the output schema
        this->values_.ResetValues();
        for (const auto& column : join_.Schema().Columns()) {
          this->values_.AddVariable(expr_translator_.Compute(column.Expr()));
        }

        // Push tuples to next operator
        if (auto parent = this->Parent()) {
          parent->get().Consume(*this);
        }

        program_.Call(increment_fn, {tuple_it});
        return i + proxy::Int32<T>(program_, 1);
      });

  // Cleanup
  program_.Call(free_it_fn, {tuple_it});
  program_.Call(free_fn, {tuple_idx_table});

  for (auto& buffer : buffers_) {
    if (buffer != nullptr) {
      buffer.reset();
    }
  }
  predicate_struct.reset();
}

template <typename T>
void SkinnerJoinTranslator<T>::Consume(OperatorTranslator<T>& src) {
  auto values = src.SchemaValues().Values();
  auto& buffer = buffers_[child_idx_];

  auto tuple_idx = buffer->Size();

  // for each predicate column on this table, insert tuple idx into
  // corresponding HT
  int next_index = 0;
  for (auto& predicate_column : predicate_columns_) {
    if (child_idx_ != predicate_column.get().GetChildIdx()) {
      continue;
    }

    indexes_[child_idx_][next_index++]->Insert(
        values[predicate_column.get().GetColumnIdx()], tuple_idx);
  }

  // insert tuple into buffer
  buffer->PushBack().Pack(values);
}

INSTANTIATE_ON_IR(SkinnerJoinTranslator);

}  // namespace kush::compile