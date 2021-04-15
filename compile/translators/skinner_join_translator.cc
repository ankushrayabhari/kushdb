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
                                   {program.PointerType(program.UI32Type())});
    auto args = program.GetFunctionArguments(*func);

    auto& idx_ptr =
        program.PointerCast(args[0], program.PointerType(program.UI32Type()));

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
  // 1. Materialize each child.
  auto child_translators = this->Children();
  auto child_operators = this->join_.Children();
  auto conditions = join_.Conditions();

  std::vector<std::unique_ptr<proxy::StructBuilder<T>>> structs;
  for (int i = 0; i < child_translators.size(); i++) {
    auto& child_translator = child_translators[i].get();
    auto& child_operator = child_operators[i].get();

    // TODO: Scans are already materialized so do nothing
    /*if (auto scan = dynamic_cast<ScanTranslator<T>*>(&child_translator)) {

    } else*/

    {
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

      // Fill buffer
      child_translator.Produce();
    }
  }

  // 2. Setup join evaluation
  // Setup struct of predicate columns
  PredicateColumnCollector collector;
  for (const auto& condition : conditions) {
    condition.get().Accept(collector);
  }
  auto predicate_columns = collector.PredicateColumns();
  auto predicate_struct = std::make_unique<proxy::StructBuilder<T>>(program_);
  for (auto& predicate_column : predicate_columns) {
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
      initial_idx_values(child_operators.size(), program_.ConstUI32(0));
  auto& idx_type = program_.UI32Type();
  auto& idx_array_type = program_.ArrayType(idx_type, child_operators.size());
  auto& idx_array =
      program_.GlobalArray(false, idx_array_type, initial_idx_values);

  // Setup flag array for each table.
  std::vector<absl::btree_set<int>> predicates_per_table(
      child_operators.size());
  for (int i = 0; i < conditions.size(); i++) {
    auto& condition = conditions[i];
    PredicateColumnCollector collector;
    condition.get().Accept(collector);
    auto predicate_columns = collector.PredicateColumns();
    for (const auto& col_ref : predicate_columns) {
      predicates_per_table[col_ref.get().GetChildIdx()].insert(i);
    }
  }

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

    proxy::VoidFunction<T>(program_, [&]() {
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
          auto& column_ref = predicate_columns[i].get();

          // Set the ColumnIdx value of the ChildIdx operator to be the
          // unpacked value.
          auto& child_translator =
              child_translators[column_ref.GetChildIdx()].get();
          child_translator.SchemaValues().SetValue(column_ref.GetColumnIdx(),
                                                   std::move(column_values[i]));
        }
      }

      // TODO: specialize this for scans, only loop over the predicate columns
      /*if (auto scan = dynamic_cast<ScanTranslator<T>*>(&child_translator)) {
        // Loop over predicate columns in column data
        program_.Return();
      } else*/
      {
        // Loop over tuples in buffer
        proxy::Vector<T>& buffer = *buffers_[current_buffer++];

        proxy::IndexLoop<T>(
            program_, [&]() { return proxy::UInt32<T>(program_, 0); },
            [&](proxy::UInt32<T>& i) { return i < buffer.Size(); },
            [&](proxy::UInt32<T>& i,
                std::function<void(proxy::UInt32<T>&)> Continue) {
              {  // Handle any predicate columns for this table
                 // Load the current tuple of table.
                auto current_table_values = buffer[i].Unpack();

                // Store each of this table's predicate column values into
                // the global_predicate_struct
                for (int k = 0; k < predicate_columns.size(); k++) {
                  auto& col_ref = predicate_columns[k].get();
                  if (col_ref.GetChildIdx() == table_idx) {
                    global_predicate_struct.Update(
                        k, *current_table_values[col_ref.GetColumnIdx()]);

                    // Additionally, update this table's values to read from
                    // the unpacked tuple instead of the old loaded value from
                    // global_predicate_struct.
                    child_translator.SchemaValues().SetValue(
                        col_ref.GetColumnIdx(),
                        std::move(
                            current_table_values[col_ref.GetColumnIdx()]));
                  }
                }
              }

              // Evaluate every predicate that references this table's
              // columns.
              for (int predicate_idx : predicates_per_table[table_idx]) {
                auto& flag_ptr = program_.GetElementPtr(
                    flag_array_type, flag_array,
                    {program_.ConstI32(0),
                     program_.ConstI32(table_predicate_to_flag_idx.at(
                         {table_idx, predicate_idx}))});
                proxy::Bool<T> flag(program_, program_.Load(flag_ptr));
                proxy::If<T>(program_, flag, [&]() {
                  auto cond =
                      expr_translator_.Compute(conditions[predicate_idx]);
                  auto next = i + proxy::UInt32<T>(program_, 1);
                  proxy::If<T>(program_, !static_cast<proxy::Bool<T>&>(*cond),
                               [&]() { Continue(next); });
                });
              }

              // Store idx into global idx array.
              auto& idx_ptr = program_.GetElementPtr(
                  idx_array_type, idx_array,
                  {program_.ConstI32(0), program_.ConstI32(table_idx)});
              program_.Store(idx_ptr, i.Get());

              // Call next table handler
              program_.Call(handler, handler_type);

              return i + proxy::UInt32<T>(program_, 1);
            });
        program_.Return();
      }
    });
  }

  // Setup global hash table that contains tuple idx
  auto& create_fn = program_.DeclareExternalFunction(
      "_ZN4kush4data19CreateTupleIdxTableEv",
      program_.PointerType(program_.I8Type()), {});
  auto& insert_fn = program_.DeclareExternalFunction(
      "_ZN4kush4data6InsertEPSt13unordered_setISt6vectorIjSaIjEESt4hashIS4_"
      "ESt8equal_toIS4_ESaIS4_EEPjj",
      program_.VoidType(),
      {program_.PointerType(program_.I8Type()),
       program_.PointerType(program_.UI32Type()), program_.UI32Type()});
  auto& free_fn = program_.DeclareExternalFunction(
      "_ZN4kush4data4FreeEPSt13unordered_setISt6vectorIjSaIjEESt4hashIS4_"
      "ESt8equal_toIS4_ESaIS4_EE",
      program_.VoidType(), {program_.PointerType(program_.I8Type())});
  auto& for_each_fn = program_.DeclareExternalFunction(
      "_ZN4kush4data7ForEachEPSt13unordered_setISt6vectorIjSaIjEESt4hashIS4_"
      "ESt8equal_toIS4_ESaIS4_EEPFvPjE",
      program_.VoidType(),
      {program_.PointerType(program_.I8Type()),
       program_.PointerType(program_.FunctionType(
           program_.VoidType(), {program_.PointerType(program_.UI32Type())}))});
  auto& tuple_idx_table_ptr = program_.GlobalPointer(
      false, program_.PointerType(program_.I8Type()),
      program_.NullPtr(program_.PointerType(program_.I8Type())));

  auto& tuple_idx_table = program_.Call(create_fn);
  program_.Store(tuple_idx_table_ptr, tuple_idx_table);

  // Setup function for each valid tuple
  proxy::VoidFunction<T> valid_tuple_handler(program_, [&]() {
    // Insert tuple idx into hash table
    auto& tuple_idx_table = program_.Load(tuple_idx_table_ptr);

    auto& tuple_idx =
        program_.GetElementPtr(idx_array_type, idx_array,
                               {program_.ConstI32(0), program_.ConstI32(0)});

    auto& num_tables = program_.ConstUI32(child_translators.size());

    program_.Call(insert_fn, {tuple_idx_table, tuple_idx, num_tables});

    program_.Return();
  });

  // Setup function that gets invoked for every output tuple
  TupleIdxHandler<T> output_handler(
      program_, [&](typename ProgramBuilder<T>::Value& tuple_idx_arr) {
        int current_buffer = 0;
        for (int i = 0; i < child_translators.size(); i++) {
          auto& child_translator = child_translators[i].get();

          auto& tuple_idx_ptr = program_.GetElementPtr(
              program_.UI32Type(), tuple_idx_arr, {program_.ConstI32(i)});
          auto tuple_idx =
              proxy::UInt32<T>(program_, program_.Load(tuple_idx_ptr));

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

        program_.Return();
      });

  // 3. Execute join

  // Toggle predicate flags and set join order
  // For now, call static order by executing 0th function
  // TODO: Setup UCT join ordering

  // Loop over tuple idx table and then output tuples from each table.
  program_.Call(for_each_fn, {tuple_idx_table, output_handler.Get()});

  // Cleanup
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
  // Last element of buffers is the materialization vector for src.
  buffers_.back()->PushBack().Pack(src.SchemaValues().Values());
}

INSTANTIATE_ON_IR(SkinnerJoinTranslator);

}  // namespace kush::compile