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

int IndexAvailableForTable(
    int table_idx, std::vector<absl::flat_hash_set<int>>& tables_per_predicate,
    std::vector<absl::btree_set<int>>& predicates_per_table,
    absl::flat_hash_set<int>& available_tables,
    absl::flat_hash_set<int>& evaluated_predicates,
    const std::vector<
        std::reference_wrapper<const kush::plan::BinaryArithmeticExpression>>&
        conditions) {
  for (int predicate_idx : predicates_per_table[table_idx]) {
    if (evaluated_predicates.contains(predicate_idx)) {
      continue;
    }

    bool can_execute = true;
    for (int table : tables_per_predicate[predicate_idx]) {
      if (!available_tables.contains(table) && table != table_idx) {
        can_execute = false;
        break;
      }
    }
    if (!can_execute) {
      continue;
    }

    const auto& predicate = conditions[predicate_idx].get();

    if (auto eq = dynamic_cast<const kush::plan::BinaryArithmeticExpression*>(
            &predicate)) {
      if (auto left_column =
              dynamic_cast<const kush::plan::ColumnRefExpression*>(
                  &eq->LeftChild())) {
        if (auto right_column =
                dynamic_cast<const kush::plan::ColumnRefExpression*>(
                    &eq->RightChild())) {
          // possible to evaluate this via index
          return predicate_idx;
        }
      }
    }
  }

  return -1;
}

proxy::Int32 RecompilingSkinnerJoinTranslator::GenerateChildLoops(
    int curr, const std::vector<int>& order, khir::ProgramBuilder& program,
    ExpressionTranslator& expr_translator,
    std::vector<std::unique_ptr<proxy::MaterializedBuffer>>&
        materialized_buffers,
    std::vector<std::unique_ptr<proxy::ColumnIndex>>& indexes,
    const std::vector<proxy::Int32>& cardinalities,
    proxy::TupleIdxTable& tuple_idx_table,
    absl::flat_hash_set<int>& evaluated_predicates,
    std::vector<absl::flat_hash_set<int>>& tables_per_predicate,
    std::vector<absl::btree_set<int>>& predicates_per_table,
    absl::flat_hash_set<int>& available_tables, khir::Value idx_array,
    khir::Value offset_array, khir::Value progress_arr,
    khir::Value table_ctr_ptr, khir::Value num_result_tuples_ptr,
    proxy::Int32 initial_budget, proxy::Bool resume_progress) {
  auto child_translators = this->Children();
  auto conditions = join_.Conditions();

  int table_idx = order[curr];
  auto& buffer = *materialized_buffers[table_idx];
  auto cardinality = cardinalities[table_idx];

  // TODO: change to look at multiple potential index predicates rather than
  // first
  auto indexed_predicate = IndexAvailableForTable(
      table_idx, tables_per_predicate, predicates_per_table, available_tables,
      evaluated_predicates, conditions);
  if (indexed_predicate >= 0) {
    // Generate a loop over the index rather than the full table
    const auto& predicate = conditions[indexed_predicate].get();

    auto eq =
        dynamic_cast<const kush::plan::BinaryArithmeticExpression*>(&predicate);
    auto left_column =
        dynamic_cast<const kush::plan::ColumnRefExpression*>(&eq->LeftChild());
    auto right_column =
        dynamic_cast<const kush::plan::ColumnRefExpression*>(&eq->RightChild());

    // Get index from table value to tuple idx
    auto table_column = table_idx == left_column->GetChildIdx()
                            ? left_column->GetColumnIdx()
                            : right_column->GetColumnIdx();

    auto it = predicate_to_index_idx_.find({table_idx, table_column});
    assert(it != predicate_to_index_idx_.end());
    auto index_idx = it->second;

    auto other_side_value = expr_translator.Compute(
        table_idx == left_column->GetChildIdx() ? *right_column : *left_column);

    return proxy::Ternary(
        program, other_side_value.IsNull(),
        [&]() {
          auto budget = initial_budget - 1;
          proxy::If(program, budget == 0, [&]() {
            auto idx_ptr = program.GetElementPtr(program.I32Type(), idx_array,
                                                 {table_idx});
            program.StoreI32(idx_ptr, (cardinality - 1).Get());
            program.StoreI32(table_ctr_ptr, program.ConstI32(table_idx));
            program.Return(program.ConstI32(-1));
          });
          return budget;
        },
        [&]() {
          auto bucket = indexes[index_idx]->GetBucket(other_side_value.Get());
          return proxy::Ternary(
              program, bucket.DoesNotExist(),
              [&]() {
                auto budget = initial_budget - 1;
                proxy::If(program, budget == 0, [&]() {
                  auto idx_ptr = program.GetElementPtr(program.I32Type(),
                                                       idx_array, {table_idx});
                  program.StoreI32(idx_ptr, (cardinality - 1).Get());
                  program.StoreI32(table_ctr_ptr, program.ConstI32(table_idx));
                  program.Return(program.ConstI32(-1));
                });
                return budget;
              },
              [&]() {
                // Loop over bucket
                auto bucket_size = bucket.Size();
                proxy::Loop loop(
                    program,
                    [&](auto& loop) {
                      auto progress_next_tuple = proxy::Ternary(
                          program, resume_progress,
                          [&]() {
                            auto progress_ptr = program.GetElementPtr(
                                program.I32Type(), progress_arr, {table_idx});
                            auto next_tuple = proxy::Int32(
                                program, program.LoadI32(progress_ptr));
                            return next_tuple;
                          },
                          [&]() { return proxy::Int32(program, 0); });
                      auto offset_next_tuple =
                          proxy::Int32(program,
                                       program.LoadI32(program.GetElementPtr(
                                           program.I32Type(), offset_array,
                                           {table_idx}))) +
                          1;
                      auto initial_next_tuple = proxy::Ternary(
                          program, offset_next_tuple > progress_next_tuple,
                          [&]() { return offset_next_tuple; },
                          [&]() { return progress_next_tuple; });
                      auto bucket_idx =
                          bucket.FastForwardToStart(initial_next_tuple);
                      loop.AddLoopVariable(bucket_idx);
                      loop.AddLoopVariable(initial_budget);

                      auto continue_resume_progress = proxy::Ternary(
                          program, resume_progress,
                          [&]() {
                            return proxy::Ternary(
                                program, bucket_idx < bucket_size,
                                [&]() {
                                  auto progress_ptr = program.GetElementPtr(
                                      program.I32Type(), progress_arr,
                                      {table_idx});
                                  auto progress_next_tuple = proxy::Int32(
                                      program, program.LoadI32(progress_ptr));

                                  auto bucket_next_tuple = bucket[bucket_idx];
                                  return bucket_next_tuple ==
                                         progress_next_tuple;
                                },
                                [&]() { return proxy::Bool(program, false); });
                          },
                          [&]() { return proxy::Bool(program, false); });
                      loop.AddLoopVariable(continue_resume_progress);
                    },
                    [&](auto& loop) {
                      auto bucket_idx =
                          loop.template GetLoopVariable<proxy::Int32>(0);
                      return bucket_idx < bucket_size;
                    },
                    [&](auto& loop) {
                      auto bucket_idx =
                          loop.template GetLoopVariable<proxy::Int32>(0);
                      auto budget =
                          loop.template GetLoopVariable<proxy::Int32>(1) - 1;
                      auto resume_progress =
                          loop.template GetLoopVariable<proxy::Bool>(2);

                      auto next_tuple = bucket[bucket_idx];
                      // guaranteed that the index condition holds
                      evaluated_predicates.insert(indexed_predicate);

                      auto idx_ptr = program.GetElementPtr(
                          program.I32Type(), idx_array, {table_idx});
                      program.StoreI32(idx_ptr, next_tuple.Get());

                      /*
                      proxy::Printer printer(program);
                      std::string x;
                      for (int i = 0; i < curr; i++) {
                        x += " ";
                      }
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
                           predicates_per_table[table_idx]) {
                        if (evaluated_predicates.contains(predicate_idx)) {
                          continue;
                        }

                        bool can_execute = true;
                        for (int table : tables_per_predicate[predicate_idx]) {
                          if (!available_tables.contains(table)) {
                            can_execute = false;
                            break;
                          }
                        }
                        if (!can_execute) {
                          continue;
                        }

                        evaluated_predicates.insert(predicate_idx);
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
                                    loop.Continue(next_tuple + 1, budget,
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
                                              next_tuple + 1, budget,
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
                                tuple_idx_table, evaluated_predicates,
                                tables_per_predicate, predicates_per_table,
                                available_tables, idx_array, offset_array,
                                progress_arr, table_ctr_ptr,
                                num_result_tuples_ptr, budget, resume_progress)
                                .ToPointer();
                      }

                      return loop.Continue(bucket_idx + 1, *next_budget,
                                           proxy::Bool(program, false));
                    });

                return loop.template GetLoopVariable<proxy::Int32>(1);
              });
        });
  }

  proxy::Loop loop(
      program,
      [&](auto& loop) {
        auto progress_next_tuple = proxy::Ternary(
            program, resume_progress,
            [&]() {
              auto progress_ptr = program.GetElementPtr(
                  program.I32Type(), progress_arr, {table_idx});
              return proxy::Int32(program, program.LoadI32(progress_ptr));
            },
            [&]() { return proxy::Int32(program, 0); });
        auto offset_next_tuple =
            proxy::Int32(program,
                         program.LoadI32(program.GetElementPtr(
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
              auto progress_ptr = program.GetElementPtr(
                  program.I32Type(), progress_arr, {table_idx});
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
            program.GetElementPtr(program.I32Type(), idx_array, {table_idx});
        program.StoreI32(idx_ptr, next_tuple.Get());

        /*
        proxy::Printer printer(program);
        std::string x;
        for (int i = 0; i < curr; i++) {
          x += " ";
        }
        auto str = proxy::String::Global(program, x);
        printer.Print(str);
        printer.Print(next_tuple);
        printer.PrintNewline();
        */

        child_translators[table_idx].get().SchemaValues().SetValues(
            buffer[next_tuple]);
        available_tables.insert(table_idx);

        for (int predicate_idx : predicates_per_table[table_idx]) {
          if (evaluated_predicates.contains(predicate_idx)) {
            continue;
          }

          bool can_execute = true;
          for (int table : tables_per_predicate[predicate_idx]) {
            if (!available_tables.contains(table)) {
              can_execute = false;
              break;
            }
          }
          if (!can_execute) {
            continue;
          }

          evaluated_predicates.insert(predicate_idx);
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
                                 tuple_idx_table, evaluated_predicates,
                                 tables_per_predicate, predicates_per_table,
                                 available_tables, idx_array, offset_array,
                                 progress_arr, table_ctr_ptr,
                                 num_result_tuples_ptr, budget, resume_progress)
                  .ToPointer();
        }

        return loop.Continue(next_tuple + 1, *next_budget,
                             proxy::Bool(program, false));
      });

  return loop.template GetLoopVariable<proxy::Int32>(1);
}

RecompilingJoinTranslator::ExecuteJoinFn
RecompilingSkinnerJoinTranslator::CompileJoinOrder(
    const std::vector<int>& order, void** materialized_buffers_raw,
    void** materialized_indexes_raw, void* tuple_idx_table_ptr) {
  auto child_translators = this->Children();
  auto child_operators = this->join_.Children();
  auto conditions = join_.Conditions();

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
      program.GetElementPtr(program.I32Type(), idx_array, {num_tables});

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
        auto progress_next_tuple = proxy::Ternary(
            program, resume_progress,
            [&]() {
              auto progress_ptr = program.GetElementPtr(
                  program.I32Type(), progress_arr, {table_idx});
              auto next_tuple =
                  proxy::Int32(program, program.LoadI32(progress_ptr));
              return next_tuple;
            },
            [&]() { return proxy::Int32(program, 0); });
        auto offset_next_tuple =
            proxy::Int32(program,
                         program.LoadI32(program.GetElementPtr(
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
              auto progress_ptr = program.GetElementPtr(
                  program.I32Type(), progress_arr, {table_idx});
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
            program.GetElementPtr(program.I32Type(), idx_array, {table_idx});
        program.StoreI32(idx_ptr, next_tuple.Get());

        /*
        proxy::Printer printer(program);
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
            cardinalities, tuple_idx_table, evaluated_predicates,
            tables_per_predicate, predicates_per_table, available_tables,
            idx_array, offset_array, progress_arr, table_ctr_ptr,
            num_result_tuples_ptr, budget, resume_progress);

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
      for (auto& predicate_column : predicate_columns_) {
        if (i != predicate_column.get().GetChildIdx()) {
          continue;
        }

        auto col_idx = predicate_column.get().GetColumnIdx();
        auto index_idx = indexes_.size();
        predicate_to_index_idx_[{i, col_idx}] = index_idx;
        if (scan->HasIndex(col_idx)) {
          indexes_.push_back(scan->GenerateIndex(col_idx));
          indexes_.back()->Init();
        } else {
          switch (predicate_column.get().Type()) {
            case catalog::SqlType::SMALLINT:
              indexes_.push_back(
                  std::make_unique<
                      proxy::MemoryColumnIndex<catalog::SqlType::SMALLINT>>(
                      program_, true));
              break;
            case catalog::SqlType::INT:
              indexes_.push_back(
                  std::make_unique<
                      proxy::MemoryColumnIndex<catalog::SqlType::INT>>(program_,
                                                                       true));
              break;
            case catalog::SqlType::BIGINT:
              indexes_.push_back(
                  std::make_unique<
                      proxy::MemoryColumnIndex<catalog::SqlType::BIGINT>>(
                      program_, true));
              break;
            case catalog::SqlType::REAL:
              indexes_.push_back(
                  std::make_unique<
                      proxy::MemoryColumnIndex<catalog::SqlType::REAL>>(
                      program_, true));
              break;
            case catalog::SqlType::DATE:
              indexes_.push_back(
                  std::make_unique<
                      proxy::MemoryColumnIndex<catalog::SqlType::DATE>>(
                      program_, true));
              break;
            case catalog::SqlType::TEXT:
              indexes_.push_back(
                  std::make_unique<
                      proxy::MemoryColumnIndex<catalog::SqlType::TEXT>>(
                      program_, true));
              break;
            case catalog::SqlType::BOOLEAN:
              indexes_.push_back(
                  std::make_unique<
                      proxy::MemoryColumnIndex<catalog::SqlType::BOOLEAN>>(
                      program_, true));
              break;
          }
          indexes_.back()->Init();
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
      // For each predicate column on this table, declare a memory index.
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
                    proxy::MemoryColumnIndex<catalog::SqlType::SMALLINT>>(
                    program_, true));
            break;
          case catalog::SqlType::INT:
            indexes_.push_back(std::make_unique<
                               proxy::MemoryColumnIndex<catalog::SqlType::INT>>(
                program_, true));
            break;
          case catalog::SqlType::BIGINT:
            indexes_.push_back(
                std::make_unique<
                    proxy::MemoryColumnIndex<catalog::SqlType::BIGINT>>(
                    program_, true));
            break;
          case catalog::SqlType::REAL:
            indexes_.push_back(
                std::make_unique<
                    proxy::MemoryColumnIndex<catalog::SqlType::REAL>>(program_,
                                                                      true));
            break;
          case catalog::SqlType::DATE:
            indexes_.push_back(
                std::make_unique<
                    proxy::MemoryColumnIndex<catalog::SqlType::DATE>>(program_,
                                                                      true));
            break;
          case catalog::SqlType::TEXT:
            indexes_.push_back(
                std::make_unique<
                    proxy::MemoryColumnIndex<catalog::SqlType::TEXT>>(program_,
                                                                      true));
            break;
          case catalog::SqlType::BOOLEAN:
            indexes_.push_back(
                std::make_unique<
                    proxy::MemoryColumnIndex<catalog::SqlType::BOOLEAN>>(
                    program_, true));
            break;
        }
      }

      // Create struct for materialization
      auto struct_builder = std::make_unique<proxy::StructBuilder>(program_);
      const auto& child_schema = child_operator.Schema().Columns();
      for (const auto& col : child_schema) {
        struct_builder->Add(col.Expr().Type(), col.Expr().Nullable());
      }
      struct_builder->Build();

      proxy::Vector buffer(program_, *struct_builder, true);

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
  proxy::TupleIdxTable tuple_idx_table(program_, true);

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
    program_.StorePtr(program_.GetElementPtr(materialized_buffer_array_type,
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
    program_.StorePtr(program_.GetElementPtr(materialized_index_array_type,
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
    program_.StoreI32(program_.GetElementPtr(cardinalities_array_type,
                                             cardinalities_array, {0, i}),
                      materialized_buffers_[i]->Size().Get());
  }

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
  auto tables_per_predicate_arr = program_.Global(
      true, true, tables_per_predicate_arr_type, tables_per_predicate_arr_init);

  // 3. Execute join
  proxy::SkinnerJoinExecutor executor(program_);
  auto compile_fn = static_cast<RecompilingJoinTranslator*>(this);
  executor.ExecuteRecompilingJoin(
      child_translators.size(), conditions.size(),
      program_.GetElementPtr(cardinalities_array_type, cardinalities_array,
                             {0, 0}),
      program_.GetElementPtr(tables_per_predicate_arr_type,
                             tables_per_predicate_arr, {0, 0}),
      compile_fn,
      program_.GetElementPtr(materialized_buffer_array_type,
                             materialized_buffer_array, {0, 0}),
      program_.GetElementPtr(materialized_index_array_type,
                             materialized_index_array, {0, 0}),
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
          program_.GetElementPtr(program_.I32Type(), tuple_idx_arr, {i});
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
  for (auto& predicate_column : predicate_columns_) {
    if (child_idx_ != predicate_column.get().GetChildIdx()) {
      continue;
    }

    auto it = predicate_to_index_idx_.find(
        {child_idx_, predicate_column.get().GetColumnIdx()});
    if (it != predicate_to_index_idx_.end()) {
      auto idx = it->second;
      const auto& value = values[predicate_column.get().GetColumnIdx()];

      // only index not null values
      proxy::If(program_, !value.IsNull(), [&]() {
        dynamic_cast<proxy::ColumnIndexBuilder*>(indexes_[idx].get())
            ->Insert(value.Get(), tuple_idx);
      });
    }
  }

  // insert tuple into buffer
  buffer_->PushBack().Pack(values);
}

}  // namespace kush::compile