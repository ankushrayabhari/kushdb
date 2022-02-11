#include "compile/translators/recompiling_skinner_join_translator.h"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_set.h"

#include "compile/forward_declare.h"
#include "compile/proxy/control_flow/if.h"
#include "compile/proxy/control_flow/loop.h"
#include "compile/proxy/function.h"
#include "compile/proxy/materialized_buffer.h"
#include "compile/proxy/skinner_join_executor.h"
#include "compile/proxy/struct.h"
#include "compile/proxy/tuple_idx_table.h"
#include "compile/proxy/value/ir_value.h"
#include "compile/proxy/vector.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "compile/translators/predicate_column_collector.h"
#include "compile/translators/recompiling_join_translator.h"
#include "compile/translators/scan_translator.h"
#include "khir/program_builder.h"
#include "khir/program_printer.h"
#include "util/vector_util.h"

namespace kush::compile {

RecompilingSkinnerJoinTranslator::RecompilingSkinnerJoinTranslator(
    const plan::SkinnerJoinOperator& join, khir::ProgramBuilder& program,
    execution::PipelineBuilder& pipeline_builder,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(join, std::move(children)),
      join_(join),
      program_(program),
      pipeline_builder_(pipeline_builder),
      expr_translator_(program_, *this),
      cache_(join_.Children().size()) {}

std::vector<std::pair<const plan::ColumnRefExpression*,
                      const plan::ColumnRefExpression*>>
IndexAvailableForTable(
    int table_idx, const absl::flat_hash_set<int>& available_tables,
    const std::vector<std::reference_wrapper<const plan::Expression>>&
        conditions,
    absl::flat_hash_set<int>& evaluated_conditions) {
  std::vector<std::pair<const plan::ColumnRefExpression*,
                        const plan::ColumnRefExpression*>>
      result;
  for (int i = 0; i < conditions.size(); i++) {
    if (evaluated_conditions.contains(i)) {
      continue;
    }

    if (auto eq = dynamic_cast<const plan::BinaryArithmeticExpression*>(
            &conditions[i].get())) {
      if (eq->OpType() == plan::BinaryArithmeticExpressionType::EQ) {
        if (auto left = dynamic_cast<const plan::ColumnRefExpression*>(
                &eq->LeftChild())) {
          if (auto right = dynamic_cast<const plan::ColumnRefExpression*>(
                  &eq->RightChild())) {
            if (left->GetChildIdx() != table_idx &&
                right->GetChildIdx() != table_idx) {
              continue;
            }

            auto other_side_value =
                left->GetChildIdx() == table_idx ? right : left;

            if (!available_tables.contains(other_side_value->GetChildIdx())) {
              continue;
            }

            auto table_value = left->GetChildIdx() == table_idx ? left : right;

            result.emplace_back(other_side_value, table_value);
            evaluated_conditions.insert(i);
          }
        }
      }
    }
  }

  return result;
}

proxy::Int32 CheckAllIndexes(
    int table_idx,
    const std::vector<std::pair<const plan::ColumnRefExpression*,
                                const plan::ColumnRefExpression*>>&
        indexed_predicates,
    khir::ProgramBuilder& program, ExpressionTranslator& expr_translator,
    const absl::flat_hash_map<std::pair<int, int>, int>& column_to_index_idx,
    std::vector<std::unique_ptr<proxy::ColumnIndex>>& indexes,
    khir::Value idx_array, proxy::Int32 cardinality,
    proxy::Int32 initial_budget, khir::Value table_ctr_ptr, int curr_index,
    std::vector<proxy::ColumnIndexBucketArray>& bucket_lists,
    const std::vector<khir::Value>& results, const int result_max_size,
    std::function<proxy::Int32(proxy::ColumnIndexBucketArray&)> handler) {
  if (curr_index == indexed_predicates.size()) {
    return handler(bucket_lists[table_idx]);
  }

  const auto& other_side_value_ref = indexed_predicates[curr_index].first;
  const auto& current_table_values_ref = indexed_predicates[curr_index].second;

  auto table_column = current_table_values_ref->GetColumnIdx();
  auto it = column_to_index_idx.find({table_idx, table_column});
  if (it == column_to_index_idx.end()) {
    throw std::runtime_error("Expected index.");
  }
  auto index_idx = it->second;

  auto other_side_value = expr_translator.Compute(*other_side_value_ref);

  return proxy::Ternary(
      program, other_side_value.IsNull(),
      [&]() {
        auto budget = initial_budget - 1;
        proxy::If(program, budget == 0, [&]() {
          auto idx_ptr =
              program.ConstGEP(program.I32Type(), idx_array, {table_idx});
          program.StoreI32(idx_ptr, (cardinality - 1).Get());
          program.StoreI32(table_ctr_ptr, program.ConstI32(table_idx));
          program.Return(program.ConstI32(-1));
        });
        return budget;
      },
      [&]() {
        auto bucket = indexes[index_idx]->GetBucket(other_side_value.Get());
        bucket_lists[table_idx].PushBack(bucket);

        return proxy::Ternary(
            program, bucket.DoesNotExist(),
            [&]() {
              auto budget = initial_budget - 1;
              proxy::If(program, budget == 0, [&]() {
                auto idx_ptr =
                    program.ConstGEP(program.I32Type(), idx_array, {table_idx});
                program.StoreI32(idx_ptr, (cardinality - 1).Get());
                program.StoreI32(table_ctr_ptr, program.ConstI32(table_idx));
                program.Return(program.ConstI32(-1));
              });
              return budget;
            },
            [&]() {
              return CheckAllIndexes(
                  table_idx, indexed_predicates, program, expr_translator,
                  column_to_index_idx, indexes, idx_array, cardinality,
                  initial_budget, table_ctr_ptr, curr_index + 1, bucket_lists,
                  results, result_max_size, handler);
            });
      });
}

bool CanExecute(
    int predicate_idx,
    const std::vector<absl::flat_hash_set<int>>& tables_per_general_condition,
    const absl::flat_hash_set<int>& available_tables) {
  bool can_execute = true;
  for (int table : tables_per_general_condition[predicate_idx]) {
    if (!available_tables.contains(table)) {
      can_execute = false;
      break;
    }
  }
  return can_execute;
}

proxy::Int32 RecompilingSkinnerJoinTranslator::GenerateChildLoops(
    int curr, const std::vector<int>& order, khir::ProgramBuilder& program,
    ExpressionTranslator& expr_translator,
    std::vector<std::unique_ptr<proxy::MaterializedBuffer>>&
        materialized_buffers,
    std::vector<std::unique_ptr<proxy::ColumnIndex>>& indexes,
    const std::vector<proxy::Int32>& cardinalities,
    proxy::TupleIdxTable& tuple_idx_table,
    absl::flat_hash_set<int>& evaluated_conditions,
    absl::flat_hash_set<int>& available_tables, khir::Value idx_array,
    khir::Value offset_array, khir::Value progress_arr,
    khir::Value table_ctr_ptr, khir::Value num_result_tuples_ptr,
    proxy::Int32 initial_budget, proxy::Bool resume_progress,
    std::vector<proxy::ColumnIndexBucketArray>& bucket_lists,
    const std::vector<khir::Value>& results, const int result_max_size) {
  auto child_translators = this->Children();
  auto conditions = join_.Conditions();

  int table_idx = order[curr];
  auto& buffer = *materialized_buffers[table_idx];
  auto cardinality = cardinalities[table_idx];

  auto indexed_predicates = IndexAvailableForTable(
      table_idx, available_tables, conditions, evaluated_conditions);
  if (!indexed_predicates.empty()) {
    bucket_lists[table_idx].Clear();
    return CheckAllIndexes(
        table_idx, indexed_predicates, program, expr_translator,
        column_to_index_idx_, indexes, idx_array, cardinality, initial_budget,
        table_ctr_ptr, 0, bucket_lists, results, result_max_size,
        [&](auto& bucket_list) {
          auto progress_next_tuple = proxy::Ternary(
              program, resume_progress,
              [&]() {
                auto progress_ptr = program.ConstGEP(program.I32Type(),
                                                     progress_arr, {table_idx});
                auto next_tuple =
                    proxy::Int32(program, program.LoadI32(progress_ptr));
                return next_tuple;
              },
              [&]() { return proxy::Int32(program, 0); });
          auto offset_next_tuple =
              proxy::Int32(program,
                           program.LoadI32(program.ConstGEP(
                               program.I32Type(), offset_array, {table_idx}))) +
              1;
          auto initial_next_tuple = proxy::Ternary(
              program, offset_next_tuple > progress_next_tuple,
              [&]() { return offset_next_tuple; },
              [&]() { return progress_next_tuple; });
          bucket_list.InitSortedIntersection(initial_next_tuple);

          auto result = program.LoadPtr(results[table_idx]);
          auto result_initial_size =
              bucket_list.PopulateSortedIntersectionResult(result,
                                                           result_max_size);

          proxy::Loop loop(
              program,
              [&](auto& loop) {
                loop.AddLoopVariable(initial_budget);
                loop.AddLoopVariable(result_initial_size);
              },
              [&](auto& loop) {
                auto result_size =
                    loop.template GetLoopVariable<proxy::Int32>(1);
                return result_size > 0;
              },
              [&](auto& loop) {
                auto budget = loop.template GetLoopVariable<proxy::Int32>(0);
                auto result_size =
                    loop.template GetLoopVariable<proxy::Int32>(1);

                proxy::Loop result_loop(
                    program,
                    [&](auto& loop) {
                      loop.AddLoopVariable(proxy::Int32(program, 0));
                      loop.AddLoopVariable(initial_budget);

                      auto continue_resume_progress = proxy::Ternary(
                          program, resume_progress,
                          [&]() {
                            auto bucket_next_tuple_ptr = program.ConstGEP(
                                program.I32Type(), result, {0});
                            auto initial_next_tuple = proxy::Int32(
                                program,
                                program.LoadI32(bucket_next_tuple_ptr));
                            return initial_next_tuple == progress_next_tuple;
                          },
                          [&]() { return proxy::Bool(program, false); });
                      loop.AddLoopVariable(continue_resume_progress);
                    },
                    [&](auto& loop) {
                      auto bucket_idx =
                          loop.template GetLoopVariable<proxy::Int32>(0);
                      return bucket_idx < result_size;
                    },
                    [&](auto& loop) {
                      auto bucket_idx =
                          loop.template GetLoopVariable<proxy::Int32>(0);
                      auto budget =
                          loop.template GetLoopVariable<proxy::Int32>(1) - 1;
                      auto resume_progress =
                          loop.template GetLoopVariable<proxy::Bool>(2);

                      auto next_tuple = SortedIntersectionResultGet(
                          program, result, bucket_idx);

                      auto idx_ptr = program.ConstGEP(program.I32Type(),
                                                      idx_array, {table_idx});
                      program.StoreI32(idx_ptr, next_tuple.Get());

                      /*
                      proxy::Printer printer(program, true);
                      std::string x;
                      for (int i = 0; i < curr; i++) {
                        x += " ";
                      }
                      x += "i";
                      auto str = proxy::String::Global(program, x);
                      printer.Print(str);
                      printer.Print(next_tuple);
                      printer.PrintNewline();
                      */

                      child_translators[table_idx]
                          .get()
                          .SchemaValues()
                          .SetValues(buffer[next_tuple]);
                      available_tables.insert(table_idx);

                      for (int predicate_idx :
                           conditions_per_table_[table_idx]) {
                        if (evaluated_conditions.contains(predicate_idx)) {
                          continue;
                        }

                        if (!CanExecute(predicate_idx, tables_per_condition_,
                                        available_tables)) {
                          continue;
                        }

                        evaluated_conditions.insert(predicate_idx);
                        auto cond =
                            expr_translator.Compute(conditions[predicate_idx]);
                        proxy::If(
                            program, cond.IsNull(),
                            [&]() {
                              // If budget, depleted return -1 and set table ctr
                              proxy::If(
                                  program, budget == 0,
                                  [&]() {
                                    program.StoreI32(
                                        table_ctr_ptr,
                                        program.ConstI32(table_idx));
                                    program.Return(program.ConstI32(-1));
                                  },
                                  [&]() {
                                    loop.Continue(bucket_idx + 1, budget,
                                                  proxy::Bool(program, false));
                                  });
                            },
                            [&]() {
                              proxy::If(
                                  program,
                                  !static_cast<proxy::Bool&>(cond.Get()),
                                  [&]() {
                                    // If budget, depleted return -1 and set
                                    // table ctr
                                    proxy::If(
                                        program, budget == 0,
                                        [&]() {
                                          program.StoreI32(
                                              table_ctr_ptr,
                                              program.ConstI32(table_idx));
                                          program.Return(program.ConstI32(-1));
                                        },
                                        [&]() {
                                          loop.Continue(
                                              bucket_idx + 1, budget,
                                              proxy::Bool(program, false));
                                        });
                                  });
                            });
                      }

                      // Valid tuple
                      std::unique_ptr<proxy::Int32> next_budget;
                      if (curr + 1 == order.size()) {
                        // Complete tuple - insert into HT
                        tuple_idx_table.Insert(
                            idx_array, proxy::Int32(program, order.size()));

                        proxy::Int32 num_result_tuples(
                            program, program.LoadI32(num_result_tuples_ptr));
                        program.StoreI32(num_result_tuples_ptr,
                                         (num_result_tuples + 1).Get());

                        // If budget depleted, return with -1 (i.e. finished
                        // entire tables)
                        proxy::If(program, budget == 0, [&]() {
                          program.StoreI32(table_ctr_ptr,
                                           program.ConstI32(table_idx));
                          program.Return(program.ConstI32(-1));
                        });

                        next_budget = budget.ToPointer();
                      } else {
                        // If budget depleted, return with -2 and store
                        // table_ctr
                        proxy::If(program, budget == 0, [&]() {
                          program.StoreI32(table_ctr_ptr,
                                           program.ConstI32(table_idx));
                          program.Return(program.ConstI32(-2));
                        });

                        // Partial tuple - loop over other tables to complete it
                        next_budget =
                            GenerateChildLoops(
                                curr + 1, order, program, expr_translator,
                                materialized_buffers, indexes, cardinalities,
                                tuple_idx_table, evaluated_conditions,
                                available_tables, idx_array, offset_array,
                                progress_arr, table_ctr_ptr,
                                num_result_tuples_ptr, budget, resume_progress,
                                bucket_lists, results, result_max_size)
                                .ToPointer();
                      }

                      return loop.Continue(bucket_idx + 1, *next_budget,
                                           proxy::Bool(program, false));
                    });

                auto next_budget =
                    result_loop.template GetLoopVariable<proxy::Int32>(1);
                auto next_result_size =
                    bucket_list.PopulateSortedIntersectionResult(
                        result, result_max_size);
                return loop.Continue(next_budget, next_result_size);
              });

          return loop.template GetLoopVariable<proxy::Int32>(0);
        });
  }

  proxy::Loop loop(
      program,
      [&](auto& loop) {
        auto progress_next_tuple = proxy::Ternary(
            program, resume_progress,
            [&]() {
              auto progress_ptr = program.ConstGEP(program.I32Type(),
                                                   progress_arr, {table_idx});
              return proxy::Int32(program, program.LoadI32(progress_ptr));
            },
            [&]() { return proxy::Int32(program, 0); });
        auto offset_next_tuple =
            proxy::Int32(program,
                         program.LoadI32(program.ConstGEP(
                             program.I32Type(), offset_array, {table_idx}))) +
            1;
        auto initial_next_tuple = proxy::Ternary(
            program, offset_next_tuple > progress_next_tuple,
            [&]() { return offset_next_tuple; },
            [&]() { return progress_next_tuple; });

        loop.AddLoopVariable(initial_next_tuple);
        loop.AddLoopVariable(initial_budget);

        auto continue_resume_progress = proxy::Ternary(
            program, resume_progress,
            [&]() {
              auto progress_ptr = program.ConstGEP(program.I32Type(),
                                                   progress_arr, {table_idx});
              auto progress_next_tuple =
                  proxy::Int32(program, program.LoadI32(progress_ptr));
              return initial_next_tuple == progress_next_tuple;
            },
            [&]() { return proxy::Bool(program, false); });
        loop.AddLoopVariable(continue_resume_progress);
      },
      [&](auto& loop) {
        auto next_tuple = loop.template GetLoopVariable<proxy::Int32>(0);
        return next_tuple < cardinality;
      },
      [&](auto& loop) {
        auto next_tuple = loop.template GetLoopVariable<proxy::Int32>(0);
        auto budget = loop.template GetLoopVariable<proxy::Int32>(1) - 1;
        auto resume_progress = loop.template GetLoopVariable<proxy::Bool>(2);

        auto idx_ptr =
            program.ConstGEP(program.I32Type(), idx_array, {table_idx});
        program.StoreI32(idx_ptr, next_tuple.Get());

        /*
        proxy::Printer printer(program, true);
        std::string x;
        for (int i = 0; i < curr; i++) {
          x += " ";
        }
        x += "l";
        auto str = proxy::String::Global(program, x);
        printer.Print(str);
        printer.Print(next_tuple);
        printer.PrintNewline();
        */

        child_translators[table_idx].get().SchemaValues().SetValues(
            buffer[next_tuple]);
        available_tables.insert(table_idx);

        for (int predicate_idx : conditions_per_table_[table_idx]) {
          if (evaluated_conditions.contains(predicate_idx)) {
            continue;
          }

          if (!CanExecute(predicate_idx, tables_per_condition_,
                          available_tables)) {
            continue;
          }

          evaluated_conditions.insert(predicate_idx);
          auto cond = expr_translator.Compute(conditions[predicate_idx]);
          proxy::If(
              program, cond.IsNull(),
              [&]() {
                // If budget, depleted return -1 and set table ctr
                proxy::If(
                    program, budget == 0,
                    [&]() {
                      program.StoreI32(table_ctr_ptr,
                                       program.ConstI32(table_idx));
                      program.Return(program.ConstI32(-1));
                    },
                    [&]() {
                      loop.Continue(next_tuple + 1, budget,
                                    proxy::Bool(program, false));
                    });
              },
              [&]() {
                proxy::If(program, !static_cast<proxy::Bool&>(cond.Get()),
                          [&]() {
                            // If budget, depleted return -1 and set table ctr
                            proxy::If(
                                program, budget == 0,
                                [&]() {
                                  program.StoreI32(table_ctr_ptr,
                                                   program.ConstI32(table_idx));
                                  program.Return(program.ConstI32(-1));
                                },
                                [&]() {
                                  loop.Continue(next_tuple + 1, budget,
                                                proxy::Bool(program, false));
                                });
                          });
              });
        }

        // Valid tuple
        std::unique_ptr<proxy::Int32> next_budget;
        if (curr + 1 == order.size()) {
          // Complete tuple - insert into HT
          tuple_idx_table.Insert(idx_array,
                                 proxy::Int32(program, order.size()));

          proxy::Int32 num_result_tuples(
              program, program.LoadI32(num_result_tuples_ptr));
          program.StoreI32(num_result_tuples_ptr,
                           (num_result_tuples + 1).Get());

          // If budget depleted, return with -1 (i.e. finished entire
          // tables)
          proxy::If(program, budget == 0, [&]() {
            program.StoreI32(table_ctr_ptr, program.ConstI32(table_idx));
            program.Return(program.ConstI32(-1));
          });

          next_budget = budget.ToPointer();
        } else {
          // If budget depleted, return with -2 and store table_ctr
          proxy::If(program, budget == 0, [&]() {
            program.StoreI32(table_ctr_ptr, program.ConstI32(table_idx));
            program.Return(program.ConstI32(-2));
          });

          // Partial tuple - loop over other tables to complete it
          next_budget =
              GenerateChildLoops(curr + 1, order, program, expr_translator,
                                 materialized_buffers, indexes, cardinalities,
                                 tuple_idx_table, evaluated_conditions,
                                 available_tables, idx_array, offset_array,
                                 progress_arr, table_ctr_ptr,
                                 num_result_tuples_ptr, budget, resume_progress,
                                 bucket_lists, results, result_max_size)
                  .ToPointer();
        }

        return loop.Continue(next_tuple + 1, *next_budget,
                             proxy::Bool(program, false));
      });

  return loop.template GetLoopVariable<proxy::Int32>(1);
}

std::unique_ptr<proxy::ColumnIndex> GenerateMemoryIndex(
    khir::ProgramBuilder& program, catalog::SqlType type) {
  switch (type) {
    case catalog::SqlType::SMALLINT:
      return std::make_unique<
          proxy::MemoryColumnIndex<catalog::SqlType::SMALLINT>>(program);
    case catalog::SqlType::INT:
      return std::make_unique<proxy::MemoryColumnIndex<catalog::SqlType::INT>>(
          program);
    case catalog::SqlType::BIGINT:
      return std::make_unique<
          proxy::MemoryColumnIndex<catalog::SqlType::BIGINT>>(program);
    case catalog::SqlType::REAL:
      return std::make_unique<proxy::MemoryColumnIndex<catalog::SqlType::REAL>>(
          program);
    case catalog::SqlType::DATE:
      return std::make_unique<proxy::MemoryColumnIndex<catalog::SqlType::DATE>>(
          program);
    case catalog::SqlType::TEXT:
      return std::make_unique<proxy::MemoryColumnIndex<catalog::SqlType::TEXT>>(
          program);
    case catalog::SqlType::BOOLEAN:
      return std::make_unique<
          proxy::MemoryColumnIndex<catalog::SqlType::BOOLEAN>>(program);
  }
}

RecompilingJoinTranslator::ExecuteJoinFn
RecompilingSkinnerJoinTranslator::CompileJoinOrder(
    const std::vector<int>& order, void** materialized_buffers_raw,
    void** materialized_indexes_raw, void* tuple_idx_table_ptr) {
  auto child_translators = this->Children();
  auto child_operators = this->join_.Children();

  for (auto child_translator : child_translators) {
    child_translator.get().SchemaValues().ResetValues();
  }

  std::string func_name = std::to_string(order[0]);
  for (int i = 1; i < order.size(); i++) {
    func_name += "-" + std::to_string(order[i]);
  }

  auto& entry = cache_.GetOrInsert(order);
  if (entry.IsCompiled()) {
    return reinterpret_cast<ExecuteJoinFn>(entry.Func(func_name));
  }

  auto& program = entry.ProgramBuilder();
  ForwardDeclare(program);

  ExpressionTranslator expr_translator(program, *this);

  auto func =
      program.CreatePublicFunction(program.I32Type(),
                                   {program.I32Type(), program.I1Type(),
                                    program.PointerType(program.I32Type()),
                                    program.PointerType(program.I32Type()),
                                    program.PointerType(program.I32Type()),
                                    program.PointerType(program.I32Type())},
                                   func_name);
  auto args = program.GetFunctionArguments(func);
  proxy::Int32 initial_budget(program, args[0]);
  proxy::Bool resume_progress(program, args[1]);
  auto progress_arr = args[2];
  auto table_ctr_ptr = args[3];
  auto num_result_tuples_ptr = args[4];
  auto idx_array = args[5];

  // TODO: as a hack, the idx array is set to be 2x the length with the
  // offset_array being the other half to avoid passing in > 6 arguments to
  // the function
  // once the asm backend supports this, we can clean this up
  int32_t num_tables = child_operators.size();
  auto offset_array =
      program.ConstGEP(program.I32Type(), idx_array, {num_tables});

  // Regenerate all child struct types/buffers in new program
  std::vector<std::unique_ptr<proxy::MaterializedBuffer>> materialized_buffers;
  for (int i = 0; i < child_operators.size(); i++) {
    materialized_buffers.push_back(materialized_buffers_[i]->Regenerate(
        program, materialized_buffers_raw[i]));
  }

  // Regenerate all indexes in new program
  std::vector<std::unique_ptr<proxy::ColumnIndex>> indexes;
  for (int i = 0; i < indexes_.size(); i++) {
    indexes.push_back(
        indexes_[i]->Regenerate(program, materialized_indexes_raw[i]));
  }

  // Create tuple idx table
  proxy::TupleIdxTable tuple_idx_table(
      program, program.PointerCast(program.ConstPtr(tuple_idx_table_ptr),
                                   program.PointerType(program.GetOpaqueType(
                                       proxy::TupleIdxTable::TypeName))));

  // Generate loop over base table
  std::vector<proxy::Int32> cardinalities;
  for (int i = 0; i < order.size(); i++) {
    cardinalities.push_back(materialized_buffers[i]->Size());
  }

  auto table_idx = order[0];
  auto& buffer = *materialized_buffers[table_idx];
  const auto& cardinality = cardinalities[table_idx];

  absl::flat_hash_set<int> available_tables;
  absl::flat_hash_set<int> evaluated_conditions;

  std::vector<proxy::ColumnIndexBucketArray> bucket_lists;
  std::vector<khir::Value> results;
  int result_max_size = 64;
  for (int i = 0; i < order.size(); i++) {
    bucket_lists.emplace_back(program, column_to_index_idx_.size());
    results.push_back(program.Alloca(program.PointerType(program.I32Type())));
    program.StorePtr(results.back(),
                     program.Alloca(program.I32Type(), result_max_size));
  }

  /*
  proxy::Printer printer(program, true);
  printer.Print(proxy::String::Global(program, func_name));
  printer.PrintNewline();
  */

  proxy::Loop loop(
      program,
      [&](auto& loop) {
        auto progress_next_tuple = proxy::Ternary(
            program, resume_progress,
            [&]() {
              auto progress_ptr = program.ConstGEP(program.I32Type(),
                                                   progress_arr, {table_idx});
              auto next_tuple =
                  proxy::Int32(program, program.LoadI32(progress_ptr));
              return next_tuple;
            },
            [&]() { return proxy::Int32(program, 0); });
        auto offset_next_tuple =
            proxy::Int32(program,
                         program.LoadI32(program.ConstGEP(
                             program.I32Type(), offset_array, {table_idx}))) +
            1;
        auto initial_next_tuple = proxy::Ternary(
            program, offset_next_tuple > progress_next_tuple,
            [&]() { return offset_next_tuple; },
            [&]() { return progress_next_tuple; });

        loop.AddLoopVariable(initial_next_tuple);
        loop.AddLoopVariable(initial_budget);

        auto continue_resume_progress = proxy::Ternary(
            program, resume_progress,
            [&]() {
              auto progress_ptr = program.ConstGEP(program.I32Type(),
                                                   progress_arr, {table_idx});
              auto progress_next_tuple =
                  proxy::Int32(program, program.LoadI32(progress_ptr));
              return initial_next_tuple == progress_next_tuple;
            },
            [&]() { return proxy::Bool(program, false); });
        loop.AddLoopVariable(continue_resume_progress);
      },
      [&](auto& loop) {
        auto next_tuple = loop.template GetLoopVariable<proxy::Int32>(0);
        return next_tuple < cardinality;
      },
      [&](auto& loop) {
        auto next_tuple = loop.template GetLoopVariable<proxy::Int32>(0);
        auto budget = loop.template GetLoopVariable<proxy::Int32>(1) - 1;
        auto resume_progress = loop.template GetLoopVariable<proxy::Bool>(2);

        auto idx_ptr =
            program.ConstGEP(program.I32Type(), idx_array, {table_idx});
        program.StoreI32(idx_ptr, next_tuple.Get());

        /*
        proxy::Printer printer(program, true);
        printer.Print(next_tuple);
        printer.PrintNewline();
        */

        proxy::If(program, budget == 0, [&]() {
          program.StoreI32(table_ctr_ptr, program.ConstI32(table_idx));
          program.Return(program.ConstI32(-2));
        });

        child_translators[table_idx].get().SchemaValues().SetValues(
            buffer[next_tuple]);
        available_tables.insert(table_idx);

        auto next_budget = GenerateChildLoops(
            1, order, program, expr_translator, materialized_buffers, indexes,
            cardinalities, tuple_idx_table, evaluated_conditions,
            available_tables, idx_array, offset_array, progress_arr,
            table_ctr_ptr, num_result_tuples_ptr, budget, resume_progress,
            bucket_lists, results, result_max_size);

        return loop.Continue(next_tuple + 1, next_budget,
                             proxy::Bool(program, false));
      });

  auto budget = loop.template GetLoopVariable<proxy::Int32>(1);
  program.Return(budget.Get());

  entry.Compile();
  return reinterpret_cast<ExecuteJoinFn>(entry.Func(func_name));
}

void RecompilingSkinnerJoinTranslator::Produce() {
  auto output_pipeline_func = program_.CurrentBlock();

  auto child_translators = this->Children();
  auto child_operators = this->join_.Children();
  auto conditions = join_.Conditions();

  int index_idx = 0;
  for (int i = 0; i < conditions.size(); i++) {
    PredicateColumnCollector collector;
    conditions[i].get().Accept(collector);
    auto cond = collector.PredicateColumns();

    for (const auto& col_ref : cond) {
      std::pair<int, int> key = {col_ref.get().GetChildIdx(),
                                 col_ref.get().GetColumnIdx()};

      if (!column_to_index_idx_.contains(key)) {
        column_to_index_idx_[key] = index_idx++;
      }
    }
  }
  indexes_ = std::vector<std::unique_ptr<proxy::ColumnIndex>>(index_idx);

  std::vector<std::unique_ptr<execution::Pipeline>> child_pipelines;
  // 1. Materialize each child.
  for (int i = 0; i < child_translators.size(); i++) {
    auto& pipeline = pipeline_builder_.CreatePipeline();
    program_.CreatePublicFunction(program_.VoidType(), {}, pipeline.Name());
    auto& child_translator = child_translators[i].get();
    auto& child_operator = child_operators[i].get();

    if (auto scan = dynamic_cast<ScanTranslator*>(&child_translator)) {
      auto disk_materialized_buffer = scan->GenerateBuffer();
      disk_materialized_buffer->Init();

      // for each indexed column on this table, scan and append to index
      for (const auto& [key, value] : column_to_index_idx_) {
        auto [child_idx, col_idx] = key;
        auto index_idx = value;
        if (i != child_idx) {
          continue;
        }

        if (scan->HasIndex(col_idx)) {
          indexes_[index_idx] = scan->GenerateIndex(col_idx);
          indexes_[index_idx]->Init();
        } else {
          auto type = child_operator.Schema().Columns()[col_idx].Expr().Type();
          indexes_[index_idx] = GenerateMemoryIndex(program_, type);
          indexes_[index_idx]->Init();
          disk_materialized_buffer->Scan(
              col_idx, [&](auto tuple_idx, auto value) {
                // only index not null values
                proxy::If(program_, !value.IsNull(), [&]() {
                  dynamic_cast<proxy::ColumnIndexBuilder*>(
                      indexes_[index_idx].get())
                      ->Insert(value.Get(), tuple_idx);
                });
              });
        }
      }

      materialized_buffers_.push_back(std::move(disk_materialized_buffer));
    } else {
      for (const auto& [key, value] : column_to_index_idx_) {
        auto [child_idx, col_idx] = key;
        auto index_idx = value;
        if (i != child_idx) {
          continue;
        }

        auto type = child_operator.Schema().Columns()[col_idx].Expr().Type();
        indexes_[index_idx] = GenerateMemoryIndex(program_, type);
        indexes_[index_idx]->Init();
      }

      // Create struct for materialization
      auto struct_builder = std::make_unique<proxy::StructBuilder>(program_);
      const auto& child_schema = child_operator.Schema().Columns();
      for (const auto& col : child_schema) {
        struct_builder->Add(col.Expr().Type(), col.Expr().Nullable());
      }
      struct_builder->Build();

      proxy::Vector buffer(program_, *struct_builder);

      // Fill buffer/indexes
      child_idx_ = i;
      buffer_ = &buffer;
      child_translator.Produce();
      buffer_ = nullptr;

      materialized_buffers_.push_back(
          std::make_unique<proxy::MemoryMaterializedBuffer>(
              program_, std::move(struct_builder), std::move(buffer)));
    }

    program_.Return();
    child_pipelines.push_back(pipeline_builder_.FinishPipeline());
  }

  auto& join_pipeline = pipeline_builder_.CreatePipeline();
  program_.CreatePublicFunction(program_.VoidType(), {}, join_pipeline.Name());
  for (auto& pipeline : child_pipelines) {
    join_pipeline.AddPredecessor(std::move(pipeline));
  }

  // 2. Setup join evaluation
  // Setup global hash table that contains tuple idx
  proxy::TupleIdxTable tuple_idx_table(program_);

  // pass all materialized buffers to the executor
  auto materialized_buffer_array_type = program_.ArrayType(
      program_.PointerType(program_.I8Type()), materialized_buffers_.size());
  auto materialized_buffer_array_init = program_.ConstantArray(
      materialized_buffer_array_type,
      std::vector<khir::Value>(
          materialized_buffers_.size(),
          program_.NullPtr(program_.PointerType(program_.I8Type()))));
  auto materialized_buffer_array =
      program_.Global(false, true, materialized_buffer_array_type,
                      materialized_buffer_array_init);
  for (int i = 0; i < materialized_buffers_.size(); i++) {
    program_.StorePtr(program_.ConstGEP(materialized_buffer_array_type,
                                        materialized_buffer_array, {0, i}),
                      materialized_buffers_[i]->Serialize());
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
    program_.StorePtr(program_.ConstGEP(materialized_index_array_type,
                                        materialized_index_array, {0, i}),
                      indexes_[i]->Serialize());
  }

  // pass all cardinalities to the executor
  auto cardinalities_array_type =
      program_.ArrayType(program_.I32Type(), materialized_buffers_.size());
  auto cardinalities_array_init = program_.ConstantArray(
      cardinalities_array_type,
      std::vector<khir::Value>(child_translators.size(), program_.ConstI32(0)));
  auto cardinalities_array = program_.Global(
      false, true, cardinalities_array_type, cardinalities_array_init);

  for (int i = 0; i < materialized_buffers_.size(); i++) {
    program_.StoreI32(program_.ConstGEP(cardinalities_array_type,
                                        cardinalities_array, {0, i}),
                      materialized_buffers_[i]->Size().Get());
  }

  // Generate the table connections.
  conditions_per_table_ =
      std::vector<absl::btree_set<int>>(child_operators.size());
  tables_per_condition_ =
      std::vector<absl::flat_hash_set<int>>(conditions.size());
  for (int pred_idx = 0; pred_idx < conditions.size(); pred_idx++) {
    PredicateColumnCollector collector;
    conditions[pred_idx].get().Accept(collector);
    auto cond = collector.PredicateColumns();

    for (auto& c : cond) {
      conditions_per_table_[c.get().GetChildIdx()].insert(pred_idx);
      tables_per_condition_[pred_idx].insert(c.get().GetChildIdx());
    }

    for (int i = 0; i < cond.size(); i++) {
      for (int j = i + 1; j < cond.size(); j++) {
        auto left_table = cond[i].get().GetChildIdx();
        auto right_table = cond[j].get().GetChildIdx();

        if (left_table != right_table) {
          table_connections_.insert({left_table, right_table});
          table_connections_.insert({right_table, left_table});
        }
      }
    }
  }

  // 3. Execute join
  proxy::SkinnerJoinExecutor executor(program_);
  auto compile_fn = static_cast<RecompilingJoinTranslator*>(this);
  executor.ExecuteRecompilingJoin(
      child_translators.size(),
      program_.ConstGEP(cardinalities_array_type, cardinalities_array, {0, 0}),
      &table_connections_, compile_fn,
      program_.ConstGEP(materialized_buffer_array_type,
                        materialized_buffer_array, {0, 0}),
      program_.ConstGEP(materialized_index_array_type, materialized_index_array,
                        {0, 0}),
      tuple_idx_table.Get());

  program_.Return();
  auto join_pipeline_obj = pipeline_builder_.FinishPipeline();

  auto& output_pipeline = pipeline_builder_.GetCurrentPipeline();
  output_pipeline.AddPredecessor(std::move(join_pipeline_obj));
  program_.SetCurrentBlock(output_pipeline_func);

  // 4. Output Tuples.
  // Loop over tuple idx table and then output tuples from each table.
  tuple_idx_table.ForEach([&](const auto& tuple_idx_arr) {
    int current_buffer = 0;
    for (int i = 0; i < child_translators.size(); i++) {
      auto& child_translator = child_translators[i].get();

      auto tuple_idx_ptr =
          program_.ConstGEP(program_.I32Type(), tuple_idx_arr, {i});
      auto tuple_idx = proxy::Int32(program_, program_.LoadI32(tuple_idx_ptr));

      auto& buffer = *materialized_buffers_[current_buffer++];
      // set the schema values of child to be the tuple_idx'th tuple of
      // current table.
      child_translator.SchemaValues().SetValues(buffer[tuple_idx]);
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

  for (auto& buffer : materialized_buffers_) {
    buffer->Reset();
  }
  for (auto& index : indexes_) {
    index->Reset();
  }
  tuple_idx_table.Reset();
}

void RecompilingSkinnerJoinTranslator::Consume(OperatorTranslator& src) {
  auto values = src.SchemaValues().Values();
  auto tuple_idx = buffer_->Size();

  // for each predicate column on this table, insert tuple idx into
  // corresponding HT
  for (int col_idx = 0; col_idx < values.size(); col_idx++) {
    auto it = column_to_index_idx_.find({child_idx_, col_idx});
    if (it != column_to_index_idx_.end()) {
      auto index_idx = it->second;
      const auto& value = values[col_idx];

      // only index not null values
      proxy::If(program_, !value.IsNull(), [&]() {
        dynamic_cast<proxy::ColumnIndexBuilder*>(indexes_[index_idx].get())
            ->Insert(value.Get(), tuple_idx);
      });
    }
  }

  // insert tuple into buffer
  buffer_->PushBack().Pack(values);
}

}  // namespace kush::compile