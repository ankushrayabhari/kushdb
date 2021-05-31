#include "compile/translators/skinner_join_translator.h"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_set.h"

#include "compile/proxy/function.h"
#include "compile/proxy/if.h"
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
#include "khir/program_builder.h"
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

SkinnerJoinTranslator::SkinnerJoinTranslator(
    const plan::SkinnerJoinOperator& join, khir::ProgramBuilder& program,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(join, std::move(children)),
      join_(join),
      program_(program),
      expr_translator_(program_, *this) {}

void SkinnerJoinTranslator::Produce() {
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
  std::vector<std::unique_ptr<proxy::StructBuilder>> structs;
  for (int i = 0; i < child_translators.size(); i++) {
    auto& child_translator = child_translators[i].get();
    auto& child_operator = child_operators[i].get();

    // Create struct for materialization
    structs.push_back(std::make_unique<proxy::StructBuilder>(program_));
    const auto& child_schema = child_operator.Schema().Columns();
    for (const auto& col : child_schema) {
      structs.back()->Add(col.Expr().Type());
    }
    structs.back()->Build();

    // Init vector of structs
    buffers_.emplace_back(program_, *structs.back());

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
                  proxy::GlobalColumnIndexImpl<catalog::SqlType::SMALLINT>>(
                  program_));
          break;
        case catalog::SqlType::INT:
          indexes_.push_back(
              std::make_unique<
                  proxy::GlobalColumnIndexImpl<catalog::SqlType::INT>>(
                  program_));
          break;
        case catalog::SqlType::BIGINT:
          indexes_.push_back(
              std::make_unique<
                  proxy::GlobalColumnIndexImpl<catalog::SqlType::BIGINT>>(
                  program_));
          break;
        case catalog::SqlType::REAL:
          indexes_.push_back(
              std::make_unique<
                  proxy::GlobalColumnIndexImpl<catalog::SqlType::REAL>>(
                  program_));
          break;
        case catalog::SqlType::DATE:
          indexes_.push_back(
              std::make_unique<
                  proxy::GlobalColumnIndexImpl<catalog::SqlType::DATE>>(
                  program_));
          break;
        case catalog::SqlType::TEXT:
          indexes_.push_back(
              std::make_unique<
                  proxy::GlobalColumnIndexImpl<catalog::SqlType::TEXT>>(
                  program_));
          break;
        case catalog::SqlType::BOOLEAN:
          indexes_.push_back(
              std::make_unique<
                  proxy::GlobalColumnIndexImpl<catalog::SqlType::BOOLEAN>>(
                  program_));
          break;
      }
    }

    // Fill buffer/indexes
    child_idx_ = i;
    child_translator.Produce();
  }

  // 2. Setup join evaluation
  // Setup struct of predicate columns
  auto predicate_struct = std::make_unique<proxy::StructBuilder>(program_);
  for (auto& predicate_column : predicate_columns_) {
    const auto& child = child_operators[predicate_column.get().GetChildIdx()];
    const auto& col =
        child.get().Schema().Columns()[predicate_column.get().GetColumnIdx()];
    auto type = col.Expr().Type();
    predicate_struct->Add(type);
  }
  predicate_struct->Build();

  auto global_predicate_struct_ref_generator = program_.Global(
      false, true, predicate_struct->Type(),
      program_.ConstantStruct(predicate_struct->Type(),
                              predicate_struct->DefaultValues())());

  // Setup idx array.
  std::vector<khir::Value> initial_idx_values(child_operators.size(),
                                              program_.ConstI32(0));
  auto idx_array_type =
      program_.ArrayType(program_.I32Type(), child_operators.size());
  auto idx_array_ref_generator = program_.Global(
      false, true, idx_array_type,
      program_.ConstantArray(idx_array_type, initial_idx_values)());

  // Setup progress array
  std::vector<khir::Value> initial_progress_values(child_operators.size(),
                                                   program_.ConstI32(-1));
  auto progress_array_type =
      program_.ArrayType(program_.I32Type(), child_operators.size());
  auto progress_array_ref_generator = program_.Global(
      false, true, progress_array_type,
      program_.ConstantArray(progress_array_type, initial_progress_values)());

  // Setup offset array
  std::vector<khir::Value> initial_offset_values(child_operators.size(),
                                                 program_.ConstI32(-1));
  auto offset_array_type =
      program_.ArrayType(program_.I32Type(), child_operators.size());
  auto offset_array_ref_generator = program_.Global(
      false, true, offset_array_type,
      program_.ConstantArray(offset_array_type, initial_offset_values)());

  // Setup table_ctr
  auto table_ctr_type = program_.ArrayType(program_.I32Type(), 1);
  auto table_ctr_ptr_ref_generator = program_.Global(
      false, true, table_ctr_type,
      program_.ConstantArray(table_ctr_type, {program_.ConstI32(0)})());

  // Setup last_table
  auto last_table_type = program_.ArrayType(program_.I32Type(), 1);
  auto last_table_ptr_ref_generator = program_.Global(
      false, true, last_table_type,
      program_.ConstantArray(last_table_type, {program_.ConstI32(0)})());

  // Setup # of result_tuples
  auto num_result_tuples_type = program_.ArrayType(program_.I32Type(), 1);
  auto num_result_tuples_ptr_ref_generator = program_.Global(
      false, true, num_result_tuples_type,
      program_.ConstantArray(num_result_tuples_type, {program_.ConstI32(0)})());

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

  auto flag_type = program_.I8Type();
  auto flag_array_type = program_.ArrayType(flag_type, total_flags);
  auto flag_array_ref_generator = program_.Global(
      false, true, flag_array_type,
      program_.ConstantArray(
          flag_array_type,
          std::vector<khir::Value>(total_flags, program_.ConstI8(0)))());

  // Setup table handlers
  auto handler_type = program_.FunctionType(
      program_.I32Type(), {program_.I32Type(), program_.I8Type()});
  auto handler_pointer_type = program_.PointerType(handler_type);
  auto handler_pointer_array_type =
      program_.ArrayType(handler_pointer_type, child_translators.size());
  auto handler_pointer_init = program_.ConstantArray(
      handler_pointer_array_type,
      std::vector<khir::Value>(child_translators.size(),
                               program_.NullPtr(handler_pointer_type)))();
  auto handler_pointer_array_ref_generator = program_.Global(
      false, true, handler_pointer_array_type, handler_pointer_init);

  std::vector<proxy::TableFunction> table_functions;
  for (int table_idx = 0; table_idx < child_translators.size(); table_idx++) {
    auto& child_translator = child_translators[table_idx].get();

    table_functions.push_back(proxy::TableFunction(
        program_, [&](auto& initial_budget, auto& resume_progress) {
          proxy::Struct global_predicate_struct(
              program_, *predicate_struct,
              global_predicate_struct_ref_generator());
          auto handler_pointer_array = handler_pointer_array_ref_generator();
          auto progress_array = progress_array_ref_generator();
          auto idx_array = idx_array_ref_generator();
          auto last_table_ptr = last_table_ptr_ref_generator();
          auto flag_array = flag_array_ref_generator();
          auto table_ctr_ptr = table_ctr_ptr_ref_generator();

          auto handler_ptr =
              program_.GetElementPtr(handler_pointer_array_type,
                                     handler_pointer_array, {0, table_idx});
          auto handler = program_.Load(handler_ptr);

          {
            // Unpack the predicate struct.
            auto column_values = global_predicate_struct.Unpack();

            // Set the schema value for each predicate column to be the ones
            // from the predicate struct.
            for (int i = 0; i < column_values.size(); i++) {
              auto& column_ref = predicate_columns_[i].get();

              // Set the ColumnIdx value of the ChildIdx operator to be the
              // unpacked value.
              auto& child_translator =
                  child_translators[column_ref.GetChildIdx()].get();
              child_translator.SchemaValues().SetValue(
                  column_ref.GetColumnIdx(), std::move(column_values[i]));
            }
          }

          // Loop over tuples in buffer
          auto buffer = buffers_[table_idx].Get();
          auto cardinality = buffer.Size();

          // Get progress
          auto progress_ptr = program_.GetElementPtr(
              progress_array_type, progress_array, {0, table_idx});
          auto progress = proxy::Int32(program_, program_.Load(progress_ptr));

          auto progress_check = proxy::If(
              program_, resume_progress != 0,
              [&]() -> std::vector<khir::Value> {
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
                auto idx_ptr = program_.GetElementPtr(idx_array_type, idx_array,
                                                      {0, table_idx});
                program_.Store(idx_ptr, next_tuple.Get());

                proxy::Int32 runtime_table_idx(program_, table_idx);

                proxy::Int32 last_table(
                    program_, program_.Load(program_.GetElementPtr(
                                  last_table_type, last_table_ptr, {0, 0})));

                std::unique_ptr<proxy::Int32> resume_progress_budget_left,
                    resume_progress_budget_right;
                auto check = proxy::If(
                    program_, runtime_table_idx != last_table,
                    [&]() -> std::vector<khir::Value> {
                      return {
                          proxy::Int32(program_,
                                       program_.Call(handler, handler_type,
                                                     {initial_budget.Get(),
                                                      resume_progress.Get()}))
                              .Get()};
                    },
                    [&]() -> std::vector<khir::Value> {
                      return {initial_budget.Get()};
                    });
                proxy::Int32 new_budget(program_, check[0]);
                proxy::If(
                    program_, new_budget < 0,
                    [&]() -> std::vector<khir::Value> {
                      program_.Return(new_budget.Get());
                      return {};
                    },
                    []() -> std::vector<khir::Value> { return {}; });

                return {next_tuple.Get(), new_budget.Get()};
              },
              [&]() -> std::vector<khir::Value> {
                return {proxy::Int32(program_, -1).Get(), initial_budget.Get()};
              });

          proxy::Int32 last_tuple(program_, progress_check[0]);
          proxy::Int32 budget(program_, progress_check[1]);

          proxy::Loop loop(
              program_,
              [&](auto& loop) {
                loop.AddLoopVariable(last_tuple);
                loop.AddLoopVariable(initial_budget);
              },
              [&](auto& loop) {
                auto last_tuple =
                    loop.template GetLoopVariable<proxy::Int32>(0);
                return last_tuple < (cardinality - 1);
              },
              [&](auto& loop) {
                auto last_tuple =
                    loop.template GetLoopVariable<proxy::Int32>(0);
                auto budget =
                    loop.template GetLoopVariable<proxy::Int32>(1) - 1;

                auto next_tuple = (last_tuple + 1).ToPointer();

                // Get next_tuple from active equality predicates
                absl::flat_hash_set<int> index_evaluated_predicates;
                for (int predicate_idx : predicates_per_table[table_idx]) {
                  const auto& predicate = conditions[predicate_idx].get();
                  if (auto eq = dynamic_cast<
                          const kush::plan::BinaryArithmeticExpression*>(
                          &predicate)) {
                    if (auto left_column = dynamic_cast<
                            const kush::plan::ColumnRefExpression*>(
                            &eq->LeftChild())) {
                      if (auto right_column = dynamic_cast<
                              const kush::plan::ColumnRefExpression*>(
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

                            auto flag_ptr = program_.GetElementPtr(
                                flag_array_type, flag_array,
                                {0, table_predicate_to_flag_idx.at(
                                        {table_idx, predicate_idx})});
                            proxy::Int8 flag_value(program_,
                                                   program_.Load(flag_ptr));

                            auto check = proxy::If(
                                program_, flag_value == 1,
                                [&]() -> std::vector<khir::Value> {
                                  auto other_side_value =
                                      expr_translator_.Compute(
                                          table_idx ==
                                                  left_column->GetChildIdx()
                                              ? *right_column
                                              : *left_column);
                                  auto next_greater_in_index =
                                      indexes_[idx]->Get()->GetNextGreater(
                                          *other_side_value, last_tuple,
                                          cardinality);

                                  auto max_check = proxy::If(
                                      program_,
                                      next_greater_in_index > (*next_tuple),
                                      [&]() -> std::vector<khir::Value> {
                                        return {next_greater_in_index.Get()};
                                      },
                                      [&]() -> std::vector<khir::Value> {
                                        return {next_tuple->Get()};
                                      });

                                  return {max_check[0]};
                                },
                                [&]() -> std::vector<khir::Value> {
                                  return {next_tuple->Get()};
                                });
                            next_tuple =
                                proxy::Int32(program_, check[0]).ToPointer();
                            index_evaluated_predicates.insert(predicate_idx);
                          }
                        }
                      }
                    }
                  }
                }

                proxy::If(
                    program_, *next_tuple == cardinality,
                    [&]() -> std::vector<khir::Value> {
                      auto last_tuple = cardinality - 1;

                      // Store last_tuple into global idx array.
                      auto idx_ptr = program_.GetElementPtr(
                          idx_array_type, idx_array, {0, table_idx});
                      program_.Store(idx_ptr, last_tuple.Get());

                      proxy::If(
                          program_, budget == 0,
                          [&]() -> std::vector<khir::Value> {
                            // Set table_ctr to be the current table
                            program_.Store(
                                program_.GetElementPtr(table_ctr_type,
                                                       table_ctr_ptr, {0, 0}),
                                program_.ConstI32(table_idx));

                            program_.Return(program_.ConstI32(-1));
                            return {};
                          },
                          []() -> std::vector<khir::Value> { return {}; });

                      program_.Return(budget.Get());
                      return {};
                    },
                    []() -> std::vector<khir::Value> { return {}; });

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
                        std::move(
                            current_table_values[col_ref.GetColumnIdx()]));
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

                  auto flag_ptr = program_.GetElementPtr(
                      flag_array_type, flag_array,
                      {0, table_predicate_to_flag_idx.at(
                              {table_idx, predicate_idx})});
                  proxy::Int8 flag_value(program_, program_.Load(flag_ptr));
                  proxy::If(
                      program_, flag_value == 1,
                      [&]() -> std::vector<khir::Value> {
                        auto cond =
                            expr_translator_.Compute(conditions[predicate_idx]);
                        proxy::If(
                            program_, !static_cast<proxy::Bool&>(*cond),
                            [&]() -> std::vector<khir::Value> {
                              auto last_tuple = next_tuple->ToPointer();

                              proxy::If(
                                  program_, budget == 0,
                                  [&]() -> std::vector<khir::Value> {
                                    // Store last_tuple into global idx array.
                                    auto idx_ptr = program_.GetElementPtr(
                                        idx_array_type, idx_array,
                                        {0, table_idx});
                                    program_.Store(idx_ptr, last_tuple->Get());

                                    // Set table_ctr to be the current table
                                    program_.Store(
                                        program_.GetElementPtr(table_ctr_type,
                                                               table_ctr_ptr,
                                                               {0, 0}),
                                        program_.ConstI32(table_idx));

                                    program_.Return(program_.ConstI32(-1));
                                    return {};
                                  },
                                  [&]() -> std::vector<khir::Value> {
                                    loop.Continue(*last_tuple, budget);
                                    return {};
                                  });
                              return {};
                            },
                            []() -> std::vector<khir::Value> { return {}; });
                        return {};
                      },
                      []() -> std::vector<khir::Value> { return {}; });
                }

                // Store idx into global idx array.
                auto idx_ptr = program_.GetElementPtr(idx_array_type, idx_array,
                                                      {0, table_idx});
                program_.Store(idx_ptr, next_tuple->Get());

                proxy::If(
                    program_, budget == 0,
                    [&]() -> std::vector<khir::Value> {
                      // Set table_ctr to be the current table
                      program_.Store(program_.GetElementPtr(
                                         table_ctr_type, table_ctr_ptr, {0, 0}),
                                     program_.ConstI32(table_idx));

                      proxy::Int32 runtime_table_idx(program_, table_idx);

                      proxy::Int32 last_table(
                          program_,
                          program_.Load(program_.GetElementPtr(
                              last_table_type, last_table_ptr, {0, 0})));

                      proxy::If(
                          program_, runtime_table_idx == last_table,
                          [&]() -> std::vector<khir::Value> {
                            program_.Call(handler, handler_type,
                                          {budget.Get(), program_.ConstI8(0)});
                            program_.Return(program_.ConstI32(-1));
                            return {};
                          },
                          []() -> std::vector<khir::Value> { return {}; });

                      program_.Return(program_.ConstI32(-2));
                      return {};
                    },
                    []() -> std::vector<khir::Value> { return {}; });

                // Valid tuple
                auto next_budget = proxy::Int32(
                    program_,
                    program_.Call(handler, handler_type,
                                  {budget.Get(), program_.ConstI8(0)}));
                proxy::If(
                    program_, next_budget < 0,
                    [&]() -> std::vector<khir::Value> {
                      program_.Return(next_budget.Get());
                      return {};
                    },
                    []() -> std::vector<khir::Value> { return {}; });
                return loop.Continue(*next_tuple, next_budget);
              });
          return loop.template GetLoopVariable<proxy::Int32>(1);
        }));
  }

  // Setup global hash table that contains tuple idx
  proxy::GlobalTupleIdxTable tuple_idx_table(program_);

  // Setup function for each valid tuple
  proxy::TableFunction valid_tuple_handler(
      program_, [&](const auto& budget, const auto& resume_progress) {
        // Insert tuple idx into hash table
        auto tuple_idx_arr = program_.GetElementPtr(
            idx_array_type, idx_array_ref_generator(), {0, 0});

        proxy::Int32 num_tables(program_, child_translators.size());
        tuple_idx_table.Get().Insert(tuple_idx_arr, num_tables);

        auto result_ptr = program_.GetElementPtr(
            num_result_tuples_type, num_result_tuples_ptr_ref_generator(),
            {0, 0});
        proxy::Int32 num_result_tuples(program_, program_.Load(result_ptr));
        program_.Store(result_ptr, (num_result_tuples + 1).Get());

        return budget;
      });

  // 3. Execute join
  // Initialize handler array with the corresponding functions for each table
  for (int i = 0; i < child_operators.size(); i++) {
    auto handler_ptr =
        program_.GetElementPtr(handler_pointer_array_type,
                               handler_pointer_array_ref_generator(), {0, i});
    program_.Store(handler_ptr,
                   {program_.GetFunctionPointer(table_functions[i].Get())});
  }

  // Serialize the table_predicate_to_flag_idx map.
  std::vector<khir::Value> table_predicate_to_flag_idx_values;
  for (const auto& [table_predicate, flag_idx] : table_predicate_to_flag_idx) {
    table_predicate_to_flag_idx_values.push_back(
        program_.ConstI32(table_predicate.first));
    table_predicate_to_flag_idx_values.push_back(
        program_.ConstI32(table_predicate.second));
    table_predicate_to_flag_idx_values.push_back(program_.ConstI32(flag_idx));
  }
  auto table_predicate_to_flag_idx_arr_type = program_.ArrayType(
      program_.I32Type(), table_predicate_to_flag_idx_values.size());
  auto table_predicate_to_flag_idx_arr_init = program_.ConstantArray(
      table_predicate_to_flag_idx_arr_type, table_predicate_to_flag_idx_values);
  auto table_predicate_to_flag_idx_arr_ref_generator =
      program_.Global(true, true, table_predicate_to_flag_idx_arr_type,
                      table_predicate_to_flag_idx_arr_init());

  // Serialize the tables_per_predicate map.
  std::vector<khir::Value> tables_per_predicate_arr_values;
  for (int i = 0; i < conditions.size(); i++) {
    tables_per_predicate_arr_values.push_back(
        program_.ConstI32(tables_per_predicate[i].size()));
  }
  for (int i = 0; i < conditions.size(); i++) {
    for (int x : tables_per_predicate[i]) {
      tables_per_predicate_arr_values.push_back(program_.ConstI32(x));
    }
  }
  auto tables_per_predicate_arr_type = program_.ArrayType(
      program_.I32Type(), tables_per_predicate_arr_values.size());
  auto tables_per_predicate_arr_init = program_.ConstantArray(
      tables_per_predicate_arr_type, tables_per_predicate_arr_values);
  auto tables_per_predicate_arr_ref_generator =
      program_.Global(true, true, tables_per_predicate_arr_type,
                      tables_per_predicate_arr_init());

  // Write out cardinalities to idx_array
  for (int i = 0; i < child_operators.size(); i++) {
    auto buffer = buffers_[i].Get();
    auto cardinality = buffer.Size();
    program_.Store(program_.GetElementPtr(idx_array_type,
                                          idx_array_ref_generator(), {0, i}),
                   cardinality.Get());
  }

  // Execute build side of skinner join
  proxy::SkinnerJoinExecutor executor(program_);

  executor.Execute({
      program_.ConstI32(child_operators.size()),
      program_.ConstI32(conditions.size()),
      program_.GetElementPtr(handler_pointer_array_type,
                             handler_pointer_array_ref_generator(), {0, 0}),
      program_.GetFunctionPointer(valid_tuple_handler.Get()),
      program_.ConstI32(table_predicate_to_flag_idx_values.size()),
      program_.GetElementPtr(table_predicate_to_flag_idx_arr_type,
                             table_predicate_to_flag_idx_arr_ref_generator(),
                             {0, 0}),
      program_.GetElementPtr(tables_per_predicate_arr_type,
                             tables_per_predicate_arr_ref_generator(), {0, 0}),
      program_.GetElementPtr(flag_array_type, flag_array_ref_generator(),
                             {0, 0}),
      program_.GetElementPtr(progress_array_type,
                             progress_array_ref_generator(), {0, 0}),
      program_.GetElementPtr(table_ctr_type, table_ctr_ptr_ref_generator(),
                             {0, 0}),
      program_.GetElementPtr(idx_array_type, idx_array_ref_generator(), {0, 0}),
      program_.GetElementPtr(last_table_type, last_table_ptr_ref_generator(),
                             {0, 0}),
      program_.GetElementPtr(num_result_tuples_type,
                             num_result_tuples_ptr_ref_generator(), {0, 0}),
      program_.GetElementPtr(offset_array_type, offset_array_ref_generator(),
                             {0, 0}),
  });

  // Loop over tuple idx table and then output tuples from each table.
  tuple_idx_table.Get().ForEach([&](const auto& tuple_idx_arr) {
    int current_buffer = 0;
    for (int i = 0; i < child_translators.size(); i++) {
      auto& child_translator = child_translators[i].get();

      auto tuple_idx_ptr =
          program_.GetElementPtr(program_.I32Type(), tuple_idx_arr, {i});
      auto tuple_idx = proxy::Int32(program_, program_.Load(tuple_idx_ptr));

      auto buffer = buffers_[current_buffer++].Get();
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

  predicate_struct.reset();
}

void SkinnerJoinTranslator::Consume(OperatorTranslator& src) {
  auto values = src.SchemaValues().Values();
  auto buffer = buffers_[child_idx_].Get();

  auto tuple_idx = buffer.Size();

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
      indexes_[idx]->Get()->Insert(
          values[predicate_column.get().GetColumnIdx()].get(), tuple_idx);
    }
  }

  // insert tuple into buffer
  buffer.PushBack().Pack(values);
}

}  // namespace kush::compile