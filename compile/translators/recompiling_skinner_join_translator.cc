#include "compile/translators/recompiling_skinner_join_translator.h"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_set.h"

#include "compile/forward_declare.h"
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
#include "compile/translators/predicate_column_collector.h"
#include "compile/translators/recompiling_join_translator.h"
#include "compile/translators/scan_translator.h"
#include "khir/program_builder.h"
#include "util/vector_util.h"

namespace kush::compile {

RecompilingSkinnerJoinTranslator::RecompilingSkinnerJoinTranslator(
    const plan::SkinnerJoinOperator& join, khir::ProgramBuilder& program,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(join, std::move(children)),
      join_(join),
      program_(program),
      expr_translator_(program_, *this),
      cache_(join_.Children().size()) {}

proxy::Int32 RecompilingSkinnerJoinTranslator::GenerateChildLoops(
    int curr, const std::vector<int>& order, khir::ProgramBuilder& program,
    ExpressionTranslator& expr_translator, std::vector<proxy::Vector>& buffers,
    std::vector<std::unique_ptr<proxy::ColumnIndex>>& indexes,
    proxy::TupleIdxTable& tuple_idx_table,
    absl::flat_hash_set<int> evaluated_predicates,
    std::vector<absl::flat_hash_set<int>>& tables_per_predicate,
    std::vector<absl::btree_set<int>>& predicates_per_table,
    absl::flat_hash_set<int> available_tables, khir::Type idx_array_type,
    khir::Value idx_array, khir::Value progress_arr, khir::Value table_ctr_ptr,
    proxy::Int32 initial_budget, proxy::Bool resume_progress) {
  auto child_translators = this->Children();
  auto conditions = join_.Conditions();

  int table_idx = order[curr];
  auto& buffer = buffers[table_idx];
  auto cardinality = buffer.Size();

  proxy::Loop loop(
      program,
      [&](auto& loop) {
        auto initial_last_tuple = proxy::If(
            program, resume_progress,
            [&]() -> std::vector<khir::Value> {
              auto progress_ptr = program.GetElementPtr(
                  program.I32Type(), progress_arr, {table_idx});
              auto progress =
                  proxy::Int32(program, program.LoadI32(progress_ptr));
              return {progress.Get()};
            },
            [&]() -> std::vector<khir::Value> {
              return {proxy::Int32(program, -1).Get()};
            });

        loop.AddLoopVariable(proxy::Int32(program, initial_last_tuple[0]));
        loop.AddLoopVariable(initial_budget);
        loop.AddLoopVariable(resume_progress);
      },
      [&](auto& loop) {
        auto last_tuple = loop.template GetLoopVariable<proxy::Int32>(0);
        return last_tuple < (cardinality - 1);
      },
      [&](auto& loop) {
        auto last_tuple = loop.template GetLoopVariable<proxy::Int32>(0);
        auto budget = loop.template GetLoopVariable<proxy::Int32>(1) - 1;
        auto resume_progress = loop.template GetLoopVariable<proxy::Bool>(2);

        auto next_tuple = (last_tuple + 1).ToPointer();

        // Get next_tuple from active equality predicates
        absl::flat_hash_set<int> index_evaluated_predicates;
        for (int predicate_idx : predicates_per_table[table_idx]) {
          if (evaluated_predicates.contains(predicate_idx)) {
            continue;
          }

          const auto& predicate = conditions[predicate_idx].get();
          if (auto eq =
                  dynamic_cast<const kush::plan::BinaryArithmeticExpression*>(
                      &predicate)) {
            if (auto left_column =
                    dynamic_cast<const kush::plan::ColumnRefExpression*>(
                        &eq->LeftChild())) {
              if (auto right_column =
                      dynamic_cast<const kush::plan::ColumnRefExpression*>(
                          &eq->RightChild())) {
                if (table_idx == left_column->GetChildIdx() ||
                    table_idx == right_column->GetChildIdx()) {
                  auto table_column = table_idx == left_column->GetChildIdx()
                                          ? left_column->GetColumnIdx()
                                          : right_column->GetColumnIdx();

                  auto it =
                      predicate_to_index_idx_.find({table_idx, table_column});
                  if (it != predicate_to_index_idx_.end()) {
                    auto idx = it->second;

                    auto other_side_value = expr_translator.Compute(
                        table_idx == left_column->GetChildIdx() ? *right_column
                                                                : *left_column);
                    auto next_greater_in_index = indexes[idx]->GetNextGreater(
                        *other_side_value, last_tuple, cardinality);

                    auto max_check = proxy::If(
                        program, next_greater_in_index > (*next_tuple),
                        [&]() -> std::vector<khir::Value> {
                          return {next_greater_in_index.Get()};
                        },
                        [&]() -> std::vector<khir::Value> {
                          return {next_tuple->Get()};
                        });

                    next_tuple =
                        proxy::Int32(program, max_check[0]).ToPointer();
                    index_evaluated_predicates.insert(predicate_idx);
                  }
                }
              }
            }
          }
        }

        proxy::If(
            program, *next_tuple == cardinality,
            [&]() -> std::vector<khir::Value> {
              // Since loop is complete check that we've depleted budget
              proxy::If(
                  program, budget == 0,
                  [&]() -> std::vector<khir::Value> {
                    // Set table_ctr to be the current table
                    program.StoreI32(table_ctr_ptr,
                                     program.ConstI32(table_idx));
                    program.Return(program.ConstI32(-1));
                    return {};
                  },
                  []() -> std::vector<khir::Value> { return {}; });

              loop.Continue(cardinality, budget, proxy::Bool(program, false));
              return {};
            },
            []() -> std::vector<khir::Value> { return {}; });

        auto tuple = buffer[*next_tuple];
        child_translators[table_idx].get().SchemaValues().SetValues(
            tuple.Unpack());
        available_tables.insert(table_idx);

        for (int predicate_idx : predicates_per_table[table_idx]) {
          if (evaluated_predicates.contains(predicate_idx)) {
            continue;
          }

          bool can_execute = true;
          for (int table : tables_per_predicate[predicate_idx]) {
            if (!available_tables.contains(table)) {
              can_execute = false;
            }
          }
          if (!can_execute) {
            continue;
          }

          // If there was only one predicate checked via index,
          // we're guaranteed it holds. Otherwise, we checked
          // multiple indexes and so we need to evaluate all
          // predicates again.
          if (index_evaluated_predicates.size() == 1 &&
              index_evaluated_predicates.contains(predicate_idx)) {
            evaluated_predicates.insert(predicate_idx);
            continue;
          }

          evaluated_predicates.insert(predicate_idx);
          auto cond = expr_translator.Compute(conditions[predicate_idx]);
          proxy::If(
              program, !static_cast<proxy::Bool&>(*cond),
              [&]() -> std::vector<khir::Value> {
                // If budget, depleted return -1 and set table ctr
                proxy::If(
                    program, budget == 0,
                    [&]() -> std::vector<khir::Value> {
                      program.StoreI32(table_ctr_ptr,
                                       program.ConstI32(table_idx));
                      program.Return(program.ConstI32(-1));
                      return {};
                    },
                    []() -> std::vector<khir::Value> { return {}; });

                loop.Continue(*next_tuple, budget, proxy::Bool(program, false));
                return {};
              },
              []() -> std::vector<khir::Value> { return {}; });
        }

        // Valid tuple

        // write idx into idx array
        auto idx_ptr =
            program.GetElementPtr(idx_array_type, idx_array, {0, table_idx});
        program.StoreI32(idx_ptr, next_tuple->Get());

        std::unique_ptr<proxy::Int32> next_budget;
        if (curr + 1 == order.size()) {
          // Complete tuple - insert into HT
          tuple_idx_table.Insert(idx_array,
                                 proxy::Int32(program, order.size()));

          // If budget depleted, return with -1 (i.e. finished entire tables)
          proxy::If(
              program, budget == 0,
              [&]() -> std::vector<khir::Value> {
                program.StoreI32(table_ctr_ptr, program.ConstI32(table_idx));
                program.Return(program.ConstI32(-1));
                return {};
              },
              []() -> std::vector<khir::Value> { return {}; });

          next_budget = budget.ToPointer();
        } else {
          // If budget depleted, return with -2 and store table_ctr
          proxy::If(
              program, budget == 0,
              [&]() -> std::vector<khir::Value> {
                program.StoreI32(table_ctr_ptr, program.ConstI32(table_idx));
                program.Return(program.ConstI32(-2));
                return {};
              },
              []() -> std::vector<khir::Value> { return {}; });

          // Partial tuple - loop over other tables to complete it
          next_budget =
              GenerateChildLoops(curr + 1, order, program, expr_translator,
                                 buffers, indexes, tuple_idx_table,
                                 evaluated_predicates, tables_per_predicate,
                                 predicates_per_table, available_tables,
                                 idx_array_type, idx_array, progress_arr,
                                 table_ctr_ptr, budget, resume_progress)
                  .ToPointer();
        }

        return loop.Continue(*next_tuple, budget, proxy::Bool(program, false));
      });

  return loop.template GetLoopVariable<proxy::Int32>(1);
}

RecompilingJoinTranslator::ExecuteJoinFn
RecompilingSkinnerJoinTranslator::CompileJoinOrder(
    const std::vector<int>& order, void** materialized_buffers,
    void** materialized_indexes, void* tuple_idx_table_ptr) {
  auto child_translators = this->Children();
  auto child_operators = this->join_.Children();
  auto conditions = join_.Conditions();

  auto& entry = cache_.GetOrInsert(order);
  if (entry.IsCompiled()) {
    return reinterpret_cast<ExecuteJoinFn>(entry.Func("compute"));
  }

  auto& program = entry.ProgramBuilder();
  ForwardDeclare(program);

  ExpressionTranslator expr_translator(program, *this);

  auto func =
      program.CreatePublicFunction(program.I32Type(),
                                   {program.I32Type(), program.I1Type(),
                                    program.PointerType(program.I32Type()),
                                    program.PointerType(program.I32Type())},
                                   "compute");
  auto args = program.GetFunctionArguments(func);
  proxy::Int32 initial_budget(program, args[0]);
  proxy::Bool resume_progress(program, args[1]);
  auto progress_arr = args[2];
  auto table_ctr_ptr = args[3];

  // Regenerate all child struct types/buffers in new program
  std::vector<std::unique_ptr<proxy::StructBuilder>> structs;
  std::vector<proxy::Vector> buffers;
  for (int i = 0; i < child_operators.size(); i++) {
    auto& child_operator = child_operators[i].get();

    structs.push_back(std::make_unique<proxy::StructBuilder>(program));
    const auto& child_schema = child_operator.Schema().Columns();
    for (const auto& col : child_schema) {
      structs.back()->Add(col.Expr().Type());
    }
    structs.back()->Build();

    buffers.emplace_back(
        program, *structs.back(),
        program.PointerCast(program.ConstPtr(materialized_buffers[i]),
                            program.PointerType(program.GetStructType(
                                proxy::Vector::VectorStructName))));
  }

  // Regenerate all indexes in new program
  std::vector<std::unique_ptr<proxy::ColumnIndex>> indexes;
  for (int i = 0; i < indexes_.size(); i++) {
    switch (indexes_[i]->Type()) {
      case catalog::SqlType::SMALLINT:
        indexes.push_back(std::make_unique<
                          proxy::ColumnIndexImpl<catalog::SqlType::SMALLINT>>(
            program, program.ConstPtr(materialized_indexes[i])));
        break;
      case catalog::SqlType::INT:
        indexes.push_back(
            std::make_unique<proxy::ColumnIndexImpl<catalog::SqlType::INT>>(
                program, program.ConstPtr(materialized_indexes[i])));
        break;
      case catalog::SqlType::BIGINT:
        indexes.push_back(
            std::make_unique<proxy::ColumnIndexImpl<catalog::SqlType::BIGINT>>(
                program, program.ConstPtr(materialized_indexes[i])));
        break;
      case catalog::SqlType::REAL:
        indexes.push_back(
            std::make_unique<proxy::ColumnIndexImpl<catalog::SqlType::REAL>>(
                program, program.ConstPtr(materialized_indexes[i])));
        break;
      case catalog::SqlType::DATE:
        indexes.push_back(
            std::make_unique<proxy::ColumnIndexImpl<catalog::SqlType::DATE>>(
                program, program.ConstPtr(materialized_indexes[i])));
        break;
      case catalog::SqlType::TEXT:
        indexes.push_back(
            std::make_unique<proxy::ColumnIndexImpl<catalog::SqlType::TEXT>>(
                program, program.ConstPtr(materialized_indexes[i])));
        break;
      case catalog::SqlType::BOOLEAN:
        indexes.push_back(
            std::make_unique<proxy::ColumnIndexImpl<catalog::SqlType::BOOLEAN>>(
                program, program.ConstPtr(materialized_indexes[i])));
        break;
    }
  }

  // Setup idx array.
  std::vector<khir::Value> initial_idx_values(child_operators.size(),
                                              program.ConstI32(0));
  auto idx_array_type =
      program.ArrayType(program.I32Type(), child_operators.size());
  auto idx_array =
      program.Global(false, true, idx_array_type,
                     program.ConstantArray(idx_array_type, initial_idx_values));

  // Create tuple idx table
  proxy::TupleIdxTable tuple_idx_table(program,
                                       program.ConstPtr(tuple_idx_table_ptr));

  // Generate loop over base table
  auto table_idx = order[0];
  auto& buffer = buffers[table_idx];
  auto cardinality = buffer.Size();

  absl::flat_hash_set<int> available_tables;
  absl::flat_hash_set<int> evaluated_predicates;
  std::vector<absl::btree_set<int>> predicates_per_table(
      child_operators.size());
  std::vector<absl::flat_hash_set<int>> tables_per_predicate(conditions.size());
  for (int i = 0; i < conditions.size(); i++) {
    auto& condition = conditions[i];

    PredicateColumnCollector collector;
    condition.get().Accept(collector);
    auto predicate_columns = collector.PredicateColumns();
    for (const auto& col_ref : predicate_columns) {
      predicates_per_table[col_ref.get().GetChildIdx()].insert(i);
      tables_per_predicate[i].insert(col_ref.get().GetChildIdx());
    }
  }

  proxy::Loop loop(
      program,
      [&](auto& loop) {
        auto initial_last_tuple = proxy::If(
            program, resume_progress,
            [&]() -> std::vector<khir::Value> {
              auto progress_ptr = program.GetElementPtr(
                  program.I32Type(), progress_arr, {table_idx});
              auto progress =
                  proxy::Int32(program, program.LoadI32(progress_ptr));
              return {progress.Get()};
            },
            [&]() -> std::vector<khir::Value> {
              return {proxy::Int32(program, -1).Get()};
            });

        loop.AddLoopVariable(proxy::Int32(program, initial_last_tuple[0]));
        loop.AddLoopVariable(initial_budget);
        loop.AddLoopVariable(resume_progress);
      },
      [&](auto& loop) {
        auto last_tuple = loop.template GetLoopVariable<proxy::Int32>(0);
        return last_tuple < (cardinality - 1);
      },
      [&](auto& loop) {
        auto last_tuple = loop.template GetLoopVariable<proxy::Int32>(0);
        auto budget = loop.template GetLoopVariable<proxy::Int32>(1) - 1;
        auto resume_progress = loop.template GetLoopVariable<proxy::Bool>(2);

        auto next_tuple = last_tuple + 1;

        proxy::If(
            program, budget == 0,
            [&]() -> std::vector<khir::Value> {
              program.StoreI32(table_ctr_ptr, program.ConstI32(table_idx));
              program.Return(program.ConstI32(-2));
              return {};
            },
            []() -> std::vector<khir::Value> { return {}; });

        auto tuple = buffer[next_tuple];
        child_translators[table_idx].get().SchemaValues().SetValues(
            tuple.Unpack());

        available_tables.insert(table_idx);
        auto idx_ptr =
            program.GetElementPtr(idx_array_type, idx_array, {0, table_idx});
        program.StoreI32(idx_ptr, next_tuple.Get());

        auto next_budget = GenerateChildLoops(
            1, order, program, expr_translator, buffers, indexes,
            tuple_idx_table, evaluated_predicates, tables_per_predicate,
            predicates_per_table, available_tables, idx_array_type, idx_array,
            progress_arr, table_ctr_ptr, budget, resume_progress);
        return loop.Continue(next_tuple, next_budget,
                             proxy::Bool(program, false));
      });

  auto budget = loop.template GetLoopVariable<proxy::Int32>(0);
  program.Return(budget.Get());

  entry.Compile();
  return reinterpret_cast<ExecuteJoinFn>(entry.Func("compute"));
}

void RecompilingSkinnerJoinTranslator::Produce() {
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
    buffers_.emplace_back(program_, *structs.back(), true);

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
                  proxy::ColumnIndexImpl<catalog::SqlType::SMALLINT>>(program_,
                                                                      true));
          break;
        case catalog::SqlType::INT:
          indexes_.push_back(
              std::make_unique<proxy::ColumnIndexImpl<catalog::SqlType::INT>>(
                  program_, true));
          break;
        case catalog::SqlType::BIGINT:
          indexes_.push_back(std::make_unique<
                             proxy::ColumnIndexImpl<catalog::SqlType::BIGINT>>(
              program_, true));
          break;
        case catalog::SqlType::REAL:
          indexes_.push_back(
              std::make_unique<proxy::ColumnIndexImpl<catalog::SqlType::REAL>>(
                  program_, true));
          break;
        case catalog::SqlType::DATE:
          indexes_.push_back(
              std::make_unique<proxy::ColumnIndexImpl<catalog::SqlType::DATE>>(
                  program_, true));
          break;
        case catalog::SqlType::TEXT:
          indexes_.push_back(
              std::make_unique<proxy::ColumnIndexImpl<catalog::SqlType::TEXT>>(
                  program_, true));
          break;
        case catalog::SqlType::BOOLEAN:
          indexes_.push_back(std::make_unique<
                             proxy::ColumnIndexImpl<catalog::SqlType::BOOLEAN>>(
              program_, true));
          break;
      }
    }

    // Fill buffer/indexes
    child_idx_ = i;
    child_translator.Produce();
  }

  // 2. Setup join evaluation
  // Setup global hash table that contains tuple idx
  proxy::TupleIdxTable tuple_idx_table(program_, true);

  // pass all materialized buffers to the executor
  auto materialized_buffer_array_type = program_.ArrayType(
      program_.PointerType(program_.I8Type()), buffers_.size());
  auto materialized_buffer_array_init = program_.ConstantArray(
      materialized_buffer_array_type,
      std::vector<khir::Value>(
          buffers_.size(),
          program_.NullPtr(program_.PointerType(program_.I8Type()))));
  auto materialized_buffer_array =
      program_.Global(false, true, materialized_buffer_array_type,
                      materialized_buffer_array_init);
  for (int i = 0; i < buffers_.size(); i++) {
    program_.StorePtr(
        program_.GetElementPtr(materialized_buffer_array_type,
                               materialized_buffer_array, {0, i}),
        program_.PointerCast(buffers_[i].Get(),
                             program_.PointerType(program_.I8Type())));
  }

  // pass all materialized indexes to the executor
  auto materialized_index_array_type = program_.ArrayType(
      program_.PointerType(program_.I8Type()), indexes_.size());
  auto materialized_index_array_init = program_.ConstantArray(
      materialized_index_array_type,
      std::vector<khir::Value>(
          indexes_.size(),
          program_.NullPtr(program_.PointerType(program_.I8Type()))));
  auto materialized_index_array =
      program_.Global(false, true, materialized_index_array_type,
                      materialized_index_array_init);
  for (int i = 0; i < indexes_.size(); i++) {
    program_.StorePtr(
        program_.GetElementPtr(materialized_index_array_type,
                               materialized_index_array, {0, i}),
        program_.PointerCast(indexes_[i]->Get(),
                             program_.PointerType(program_.I8Type())));
  }

  // 3. Execute join
  proxy::SkinnerJoinExecutor executor(program_);
  auto compile_fn = static_cast<RecompilingJoinTranslator*>(this);
  executor.ExecuteRecompilingJoin(
      child_translators.size(), compile_fn,
      program_.GetElementPtr(materialized_buffer_array_type,
                             materialized_buffer_array, {0, 0}),
      program_.GetElementPtr(materialized_index_array_type,
                             materialized_index_array, {0, 0}),
      tuple_idx_table.Get());

  // 4. Output Tuples.
  // Loop over tuple idx table and then output tuples from each table.
  tuple_idx_table.ForEach([&](const auto& tuple_idx_arr) {
    int current_buffer = 0;
    for (int i = 0; i < child_translators.size(); i++) {
      auto& child_translator = child_translators[i].get();

      auto tuple_idx_ptr =
          program_.GetElementPtr(program_.I32Type(), tuple_idx_arr, {i});
      auto tuple_idx = proxy::Int32(program_, program_.LoadI32(tuple_idx_ptr));

      auto& buffer = buffers_[current_buffer++];
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
}

void RecompilingSkinnerJoinTranslator::Consume(OperatorTranslator& src) {
  auto values = src.SchemaValues().Values();
  auto& buffer = buffers_[child_idx_];
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
      indexes_[idx]->Insert(values[predicate_column.get().GetColumnIdx()].get(),
                            tuple_idx);
    }
  }

  // insert tuple into buffer
  buffer.PushBack().Pack(values);
}

}  // namespace kush::compile