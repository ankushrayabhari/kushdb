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
#include "compile/proxy/skinner_join_executor.h"
#include "compile/proxy/string.h"
#include "compile/proxy/struct.h"
#include "compile/proxy/tuple_idx_table.h"
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

    // For each predicate column on this table, declare an index.
    for (auto& predicate_column : predicate_columns_) {
      if (i != predicate_column.get().GetChildIdx()) {
        continue;
      }

      predicate_to_index_idx_[{predicate_column.get().GetChildIdx(),
                               predicate_column.get().GetColumnIdx()}] =
          indexes_.size();

      switch (predicate_column.get().Type()) {
        case catalog::SqlType::SMALLINT:
          indexes_.push_back(
              std::make_unique<
                  proxy::ColumnIndexImpl<T, catalog::SqlType::SMALLINT>>(
                  program_, true));
          break;
        case catalog::SqlType::INT:
          indexes_.push_back(std::make_unique<
                             proxy::ColumnIndexImpl<T, catalog::SqlType::INT>>(
              program_, true));
          break;
        case catalog::SqlType::BIGINT:
          indexes_.push_back(
              std::make_unique<
                  proxy::ColumnIndexImpl<T, catalog::SqlType::BIGINT>>(program_,
                                                                       true));
          break;
        case catalog::SqlType::REAL:
          indexes_.push_back(std::make_unique<
                             proxy::ColumnIndexImpl<T, catalog::SqlType::REAL>>(
              program_, true));
          break;
        case catalog::SqlType::DATE:
          indexes_.push_back(std::make_unique<
                             proxy::ColumnIndexImpl<T, catalog::SqlType::DATE>>(
              program_, true));
          break;
        case catalog::SqlType::TEXT:
          indexes_.push_back(std::make_unique<
                             proxy::ColumnIndexImpl<T, catalog::SqlType::TEXT>>(
              program_, true));
          break;
        case catalog::SqlType::BOOLEAN:
          indexes_.push_back(
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
  auto& idx_array_type =
      program_.ArrayType(program_.I32Type(), child_operators.size());
  auto& idx_array =
      program_.GlobalArray(false, idx_array_type, initial_idx_values);

  // Setup progress array
  std::vector<std::reference_wrapper<typename ProgramBuilder<T>::Value>>
      initial_progress_values(child_operators.size(), program_.ConstI32(-1));
  auto& progress_array_type =
      program_.ArrayType(program_.I32Type(), child_operators.size());
  auto& progress_array =
      program_.GlobalArray(false, progress_array_type, initial_progress_values);

  // Setup offset array
  std::vector<std::reference_wrapper<typename ProgramBuilder<T>::Value>>
      initial_offset_values(child_operators.size(), program_.ConstI32(-1));
  auto& offset_array_type =
      program_.ArrayType(program_.I32Type(), child_operators.size());
  auto& offset_array =
      program_.GlobalArray(false, offset_array_type, initial_offset_values);

  // Setup table_ctr
  auto& table_ctr_type = program_.ArrayType(program_.I32Type(), 1);
  auto& table_ctr_ptr =
      program_.GlobalArray(false, table_ctr_type, {program_.ConstI32(0)});

  // Setup last_table
  auto& last_table_type = program_.ArrayType(program_.I32Type(), 1);
  auto& last_table_ptr =
      program_.GlobalArray(false, table_ctr_type, {program_.ConstI32(0)});

  // Setup # of result_tuples
  auto& num_result_tuples_type = program_.ArrayType(program_.I32Type(), 1);
  auto& num_result_tuples_ptr =
      program_.GlobalArray(false, table_ctr_type, {program_.ConstI32(0)});

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
      initial_flag_values(total_flags, program_.ConstI8(0));
  auto& flag_type = program_.I8Type();
  auto& flag_array_type = program_.ArrayType(flag_type, total_flags);
  auto& flag_array =
      program_.GlobalArray(false, flag_array_type, initial_flag_values);

  // Setup table handlers
  auto& handler_type = program_.FunctionType(
      program_.I32Type(), {program_.I32Type(), program_.I8Type()});
  auto& handler_pointer_type = program_.PointerType(handler_type);
  auto& handler_pointer_array_type =
      program_.ArrayType(handler_pointer_type, child_translators.size());
  std::vector<std::reference_wrapper<typename ProgramBuilder<T>::Value>>
      initial_values(child_translators.size(),
                     program_.NullPtr(handler_pointer_type));
  auto& handler_pointer_array =
      program_.GlobalArray(false, handler_pointer_array_type, initial_values);

  std::vector<proxy::TableFunction<T>> table_functions;
  for (int table_idx = 0; table_idx < child_translators.size(); table_idx++) {
    auto& child_translator = child_translators[table_idx].get();

    table_functions.push_back(proxy::TableFunction<
                              T>(program_, [&](auto& initial_budget,
                                               auto& resume_progress) {
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

      // Loop over tuples in buffer
      proxy::Vector<T>& buffer = *buffers_[table_idx];
      auto cardinality = proxy::Int32<T>(program_, buffer.Size().Get());

      // Get progress
      auto& progress_ptr = program_.GetElementPtr(
          progress_array_type, progress_array,
          {program_.ConstI32(0), program_.ConstI32(table_idx)});
      auto progress = proxy::Int32<T>(program_, program_.Load(progress_ptr));

      std::unique_ptr<proxy::Int32<T>> last_tuple_left, last_tuple_right;
      std::unique_ptr<proxy::Int32<T>> budget_left, budget_right;
      proxy::If<T> progress_check(
          program_, resume_progress != 0,
          [&]() {
            auto next_tuple = progress;

            // Load the current tuple of table.
            auto current_table_values = buffer[next_tuple].Unpack();

            // Store each of this table's predicate column values into
            // the global_predicate_struct
            for (int k = 0; k < predicate_columns_.size(); k++) {
              auto& col_ref = predicate_columns_[k].get();
              if (col_ref.GetChildIdx() == table_idx) {
                global_predicate_struct.Update(
                    k, *current_table_values[col_ref.GetColumnIdx()]);
              }
            }

            // Store idx into global idx array.
            auto& idx_ptr = program_.GetElementPtr(
                idx_array_type, idx_array,
                {program_.ConstI32(0), program_.ConstI32(table_idx)});
            program_.Store(idx_ptr, next_tuple.Get());

            proxy::Int32<T> runtime_table_idx(program_, table_idx);

            proxy::Int32<T> last_table(
                program_, program_.Load(program_.GetElementPtr(
                              last_table_type, last_table_ptr,
                              {program_.ConstI32(0), program_.ConstI32(0)})));

            std::unique_ptr<proxy::Int32<T>> resume_progress_budget_left,
                resume_progress_budget_right;
            proxy::If<T> check(
                program_, runtime_table_idx != last_table,
                [&]() {
                  resume_progress_budget_left =
                      proxy::Int32<T>(program_,
                                      program_.Call(handler, handler_type,
                                                    {initial_budget.Get(),
                                                     resume_progress.Get()}))
                          .ToPointer();
                },
                [&]() {
                  resume_progress_budget_right = initial_budget.ToPointer();
                });
            auto new_budget = check.Phi(*resume_progress_budget_left,
                                        *resume_progress_budget_right);
            proxy::If<T>(program_, new_budget < 0,
                         [&]() { program_.Return(new_budget.Get()); });

            last_tuple_left = next_tuple.ToPointer();
            budget_left = new_budget.ToPointer();
          },
          [&]() {
            last_tuple_right = proxy::Int32<T>(program_, -1).ToPointer();
            budget_right = initial_budget.ToPointer();
          });

      auto last_tuple = progress_check.Phi(*last_tuple_left, *last_tuple_right);
      auto budget = progress_check.Phi(*budget_left, *budget_right);

      proxy::Loop<T> loop(
          program_,
          [&](auto& loop) {
            loop.AddLoopVariable(last_tuple);
            loop.AddLoopVariable(initial_budget);
          },
          [&](auto& loop) {
            auto last_tuple = loop.template GetLoopVariable<proxy::Int32<T>>(0);
            return last_tuple < (cardinality - 1);
          },
          [&](auto& loop) {
            auto last_tuple = loop.template GetLoopVariable<proxy::Int32<T>>(0);
            auto budget = loop.template GetLoopVariable<proxy::Int32<T>>(1) - 1;

            auto next_tuple = (last_tuple + 1).ToPointer();

            // Get next_tuple from active equality predicates
            absl::flat_hash_set<int> index_evaluated_predicates;
            for (int predicate_idx : predicates_per_table[table_idx]) {
              const auto& predicate = conditions[predicate_idx].get();
              if (auto eq = dynamic_cast<
                      const kush::plan::BinaryArithmeticExpression*>(
                      &predicate)) {
                if (auto left_column =
                        dynamic_cast<const kush::plan::ColumnRefExpression*>(
                            &eq->LeftChild())) {
                  if (auto right_column =
                          dynamic_cast<const kush::plan::ColumnRefExpression*>(
                              &eq->RightChild())) {
                    if (table_idx == left_column->GetChildIdx() ||
                        table_idx == right_column->GetChildIdx()) {
                      auto table_column =
                          table_idx == left_column->GetChildIdx()
                              ? left_column->GetColumnIdx()
                              : right_column->GetColumnIdx();

                      auto it = predicate_to_index_idx_.find(
                          {table_idx, table_column});
                      if (it != predicate_to_index_idx_.end()) {
                        auto idx = it->second;

                        auto& flag_ptr = program_.GetElementPtr(
                            flag_array_type, flag_array,
                            {program_.ConstI32(0),
                             program_.ConstI32(table_predicate_to_flag_idx.at(
                                 {table_idx, predicate_idx}))});
                        proxy::Int8<T> flag_value(program_,
                                                  program_.Load(flag_ptr));

                        std::unique_ptr<proxy::Int32<T>> next_tuple_result;
                        proxy::If<T> check(program_, flag_value == 1, [&]() {
                          auto other_side_value = expr_translator_.Compute(
                              table_idx == left_column->GetChildIdx()
                                  ? *right_column
                                  : *left_column);
                          auto next_greater_in_index =
                              indexes_[idx]->GetNextGreater(
                                  *other_side_value, last_tuple, cardinality);

                          proxy::If<T> max_check(
                              program_, next_greater_in_index > (*next_tuple),
                              [&]() {
                                next_tuple_result =
                                    next_greater_in_index.ToPointer();
                              });

                          next_tuple_result =
                              max_check.Phi(*next_tuple, *next_tuple_result)
                                  .ToPointer();
                        });
                        next_tuple = check.Phi(*next_tuple, *next_tuple_result)
                                         .ToPointer();

                        index_evaluated_predicates.insert(predicate_idx);
                      }
                    }
                  }
                }
              }
            }

            proxy::If<T>(program_, *next_tuple == cardinality, [&]() {
              auto last_tuple = cardinality - 1;

              // Store last_tuple into global idx array.
              auto& idx_ptr = program_.GetElementPtr(
                  idx_array_type, idx_array,
                  {program_.ConstI32(0), program_.ConstI32(table_idx)});
              program_.Store(idx_ptr, last_tuple.Get());

              proxy::If<T>(program_, budget == 0, [&] {
                // Set table_ctr to be the current table
                program_.Store(
                    program_.GetElementPtr(
                        table_ctr_type, table_ctr_ptr,
                        {program_.ConstI32(0), program_.ConstI32(0)}),
                    program_.ConstI32(table_idx));

                program_.Return(program_.ConstI32(-1));
              });

              program_.Return(budget.Get());
            });

            // Load the current tuple of table.
            auto current_table_values = buffer[*next_tuple].Unpack();

            // Store each of this table's predicate column values into
            // the global_predicate_struct
            for (int k = 0; k < predicate_columns_.size(); k++) {
              auto& col_ref = predicate_columns_[k].get();
              if (col_ref.GetChildIdx() == table_idx) {
                global_predicate_struct.Update(
                    k, *current_table_values[col_ref.GetColumnIdx()]);

                // Additionally, update this table's values to read from
                // the unpacked tuple instead of the old loaded value
                // from global_predicate_struct.
                child_translator.SchemaValues().SetValue(
                    col_ref.GetColumnIdx(),
                    std::move(current_table_values[col_ref.GetColumnIdx()]));
              }
            }

            for (int predicate_idx : predicates_per_table[table_idx]) {
              // If there was only one predicate checked via index,
              // we're guaranteed it holds. Otherwise, we checked
              // multiple indexes and so we need to evaluate all
              // predicates again.
              if (index_evaluated_predicates.size() == 1 &&
                  index_evaluated_predicates.contains(predicate_idx)) {
                continue;
              }

              auto& flag_ptr = program_.GetElementPtr(
                  flag_array_type, flag_array,
                  {program_.ConstI32(0),
                   program_.ConstI32(table_predicate_to_flag_idx.at(
                       {table_idx, predicate_idx}))});
              proxy::Int8<T> flag_value(program_, program_.Load(flag_ptr));
              proxy::If<T>(program_, flag_value == 1, [&]() {
                auto cond = expr_translator_.Compute(conditions[predicate_idx]);
                proxy::If<T>(
                    program_, !static_cast<proxy::Bool<T>&>(*cond), [&]() {
                      auto last_tuple = next_tuple->ToPointer();

                      proxy::If<T>(
                          program_, budget == 0,
                          [&]() {
                            // Store last_tuple into global idx array.
                            auto& idx_ptr = program_.GetElementPtr(
                                idx_array_type, idx_array,
                                {program_.ConstI32(0),
                                 program_.ConstI32(table_idx)});
                            program_.Store(idx_ptr, last_tuple->Get());

                            // Set table_ctr to be the current table
                            program_.Store(program_.GetElementPtr(
                                               table_ctr_type, table_ctr_ptr,
                                               {program_.ConstI32(0),
                                                program_.ConstI32(0)}),
                                           program_.ConstI32(table_idx));

                            program_.Return(program_.ConstI32(-1));
                          },
                          [&]() {
                            loop.Continue({*last_tuple, budget});
                          });
                    });
              });
            }

            // Store idx into global idx array.
            auto& idx_ptr = program_.GetElementPtr(
                idx_array_type, idx_array,
                {program_.ConstI32(0), program_.ConstI32(table_idx)});
            program_.Store(idx_ptr, next_tuple->Get());

            proxy::If<T>(program_, budget == 0, [&]() {
              // Set table_ctr to be the current table
              program_.Store(program_.GetElementPtr(
                                 table_ctr_type, table_ctr_ptr,
                                 {program_.ConstI32(0), program_.ConstI32(0)}),
                             program_.ConstI32(table_idx));

              proxy::Int32<T> runtime_table_idx(program_, table_idx);

              proxy::Int32<T> last_table(
                  program_, program_.Load(program_.GetElementPtr(
                                last_table_type, last_table_ptr,
                                {program_.ConstI32(0), program_.ConstI32(0)})));

              proxy::If<T>(program_, runtime_table_idx == last_table, [&]() {
                program_.Call(handler, handler_type,
                              {budget.Get(), program_.ConstI8(0)});
                program_.Return(program_.ConstI32(-1));
              });

              program_.Return(program_.ConstI32(-2));
            });

            // Valid tuple
            {
              auto next_budget = proxy::Int32<T>(
                  program_, program_.Call(handler, handler_type,
                                          {budget.Get(), program_.ConstI8(0)}));
              proxy::If<T>(program_, next_budget < 0,
                           [&]() { program_.Return(next_budget.Get()); });
              {
                using value = std::unique_ptr<proxy::Value<T>>;
                value last_tuple = next_tuple->ToPointer();
                value budget = next_budget.ToPointer();
                return util::MakeVector(std::move(last_tuple),
                                        std::move(budget));
              }
            }
          });
      return loop.template GetLoopVariable<proxy::Int32<T>>(1);
    }));
  }

  // Setup global hash table that contains tuple idx
  proxy::TupleIdxTable<T> tuple_idx_table(program_);

  // Setup function for each valid tuple
  proxy::TableFunction<T> valid_tuple_handler(
      program_, [&](auto& budget, auto& resume_progress) {
        // Insert tuple idx into hash table
        auto& tuple_idx_arr = program_.GetElementPtr(
            idx_array_type, idx_array,
            {program_.ConstI32(0), program_.ConstI32(0)});

        proxy::Int32<T> num_tables(program_, child_translators.size());
        tuple_idx_table.Insert(tuple_idx_arr, num_tables);

        auto& result_ptr = program_.GetElementPtr(
            num_result_tuples_type, num_result_tuples_ptr,
            {program_.ConstI32(0), program_.ConstI32(0)});
        proxy::Int32<T> num_result_tuples(program_, program_.Load(result_ptr));
        program_.Store(result_ptr, (num_result_tuples + 1).Get());

        return budget;
      });

  // 3. Execute join
  // Initialize handler array with the corresponding functions for each table
  for (int i = 0; i < child_operators.size(); i++) {
    auto& handler_ptr = program_.GetElementPtr(
        handler_pointer_array_type, handler_pointer_array,
        {program_.ConstI32(0), program_.ConstI32(i)});
    program_.Store(handler_ptr, {table_functions[i].Get()});
  }

  // Serialize the table_predicate_to_flag_idx map.
  std::vector<std::reference_wrapper<typename ProgramBuilder<T>::Value>>
      table_predicate_to_flag_idx_values;
  for (const auto& [table_predicate, flag_idx] : table_predicate_to_flag_idx) {
    table_predicate_to_flag_idx_values.push_back(
        program_.ConstI32(table_predicate.first));
    table_predicate_to_flag_idx_values.push_back(
        program_.ConstI32(table_predicate.second));
    table_predicate_to_flag_idx_values.push_back(program_.ConstI32(flag_idx));
  }
  auto& table_predicate_to_flag_idx_arr_type = program_.ArrayType(
      program_.I32Type(), table_predicate_to_flag_idx_values.size());
  auto& table_predicate_to_flag_idx_arr =
      program_.GlobalArray(true, table_predicate_to_flag_idx_arr_type,
                           table_predicate_to_flag_idx_values);

  // Serialize the tables_per_predicate map.
  std::vector<std::reference_wrapper<typename ProgramBuilder<T>::Value>>
      tables_per_predicate_arr_values;
  for (int i = 0; i < conditions.size(); i++) {
    tables_per_predicate_arr_values.push_back(
        program_.ConstI32(tables_per_predicate[i].size()));
  }
  for (int i = 0; i < conditions.size(); i++) {
    for (int x : tables_per_predicate[i]) {
      tables_per_predicate_arr_values.push_back(program_.ConstI32(x));
    }
  }
  auto& tables_per_predicate_arr_type = program_.ArrayType(
      program_.I32Type(), tables_per_predicate_arr_values.size());
  auto& tables_per_predicate_arr = program_.GlobalArray(
      true, tables_per_predicate_arr_type, tables_per_predicate_arr_values);

  // Write out cardinalities to idx_array
  for (int i = 0; i < child_operators.size(); i++) {
    proxy::Vector<T>& buffer = *buffers_[i];
    auto cardinality = proxy::Int32<T>(program_, buffer.Size().Get());
    program_.Store(
        program_.GetElementPtr(idx_array_type, idx_array,
                               {program_.ConstI32(0), program_.ConstI32(i)}),
        cardinality.Get());
  }

  // Execute build side of skinner join
  proxy::SkinnerJoinExecutor<T> executor(program_);

  executor.Execute({
      program_.ConstI32(child_operators.size()),
      program_.ConstI32(conditions.size()),
      program_.GetElementPtr(handler_pointer_array_type, handler_pointer_array,
                             {program_.ConstI32(0), program_.ConstI32(0)}),
      valid_tuple_handler.Get(),
      program_.ConstI32(table_predicate_to_flag_idx_values.size()),
      program_.GetElementPtr(table_predicate_to_flag_idx_arr_type,
                             table_predicate_to_flag_idx_arr,
                             {program_.ConstI32(0), program_.ConstI32(0)}),
      program_.GetElementPtr(tables_per_predicate_arr_type,
                             tables_per_predicate_arr,
                             {program_.ConstI32(0), program_.ConstI32(0)}),
      program_.GetElementPtr(flag_array_type, flag_array,
                             {program_.ConstI32(0), program_.ConstI32(0)}),
      program_.GetElementPtr(progress_array_type, progress_array,
                             {program_.ConstI32(0), program_.ConstI32(0)}),
      program_.GetElementPtr(table_ctr_type, table_ctr_ptr,
                             {program_.ConstI32(0), program_.ConstI32(0)}),
      program_.GetElementPtr(idx_array_type, idx_array,
                             {program_.ConstI32(0), program_.ConstI32(0)}),
      program_.GetElementPtr(last_table_type, last_table_ptr,
                             {program_.ConstI32(0), program_.ConstI32(0)}),
      program_.GetElementPtr(num_result_tuples_type, num_result_tuples_ptr,
                             {program_.ConstI32(0), program_.ConstI32(0)}),
      program_.GetElementPtr(offset_array_type, offset_array,
                             {program_.ConstI32(0), program_.ConstI32(0)}),
  });

  // Loop over tuple idx table and then output tuples from each table.
  tuple_idx_table.ForEach([&](auto& tuple_idx_arr) {
    int current_buffer = 0;
    for (int i = 0; i < child_translators.size(); i++) {
      auto& child_translator = child_translators[i].get();

      auto& tuple_idx_ptr = program_.GetElementPtr(
          program_.I32Type(), tuple_idx_arr, {program_.ConstI32(i)});
      auto tuple_idx = proxy::Int32<T>(program_, program_.Load(tuple_idx_ptr));

      proxy::Vector<T>& buffer = *buffers_[current_buffer++];
      // set the schema values of child to be the tuple_idx'th tuple of
      // current table.
      child_translator.SchemaValues().SetValues(buffer[tuple_idx].Unpack());
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
  });

  for (auto& index : indexes_) {
    index.reset();
  }

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
  for (auto& predicate_column : predicate_columns_) {
    if (child_idx_ != predicate_column.get().GetChildIdx()) {
      continue;
    }

    auto it = predicate_to_index_idx_.find(
        {child_idx_, predicate_column.get().GetColumnIdx()});
    if (it != predicate_to_index_idx_.end()) {
      auto idx = it->second;
      indexes_[idx]->Insert(values[predicate_column.get().GetColumnIdx()].get(),
                            tuple_idx);
    }
  }

  // insert tuple into buffer
  buffer->PushBack().Pack(values);
}

INSTANTIATE_ON_IR(SkinnerJoinTranslator);

}  // namespace kush::compile