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
#include "compile/proxy/disk_column_index.h"
#include "compile/proxy/function.h"
#include "compile/proxy/materialized_buffer.h"
#include "compile/proxy/memory_column_index.h"
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

class RecompilingTableFunction {
 public:
  RecompilingTableFunction(
      std::string name, khir::ProgramBuilder& program,
      std::function<proxy::Int32(proxy::Int32&, proxy::Bool&)> body) {
    func_ = program.CreateNamedFunction(
        program.I32Type(), {program.I32Type(), program.I1Type()}, name);

    auto args = program.GetFunctionArguments(func_.value());
    proxy::Int32 budget(program, args[0]);
    proxy::Bool resume_progress(program, args[1]);

    auto x = body(budget, resume_progress);
    program.Return(x.Get());
  }

  khir::FunctionRef Get() { return func_.value(); }

 private:
  std::optional<khir::FunctionRef> func_;
  static int table_;
};

bool EqualityPredicate(std::reference_wrapper<const plan::Expression> expr) {
  if (auto eq =
          dynamic_cast<const plan::BinaryArithmeticExpression*>(&expr.get())) {
    if (eq->OpType() == plan::BinaryArithmeticExpressionType::EQ) {
      if (auto l = dynamic_cast<const plan::ColumnRefExpression*>(
              &eq->LeftChild())) {
        if (auto r = dynamic_cast<const plan::ColumnRefExpression*>(
                &eq->RightChild())) {
          return true;
        }
      }
    }
  }
  return false;
}

RecompilingSkinnerJoinTranslator::RecompilingSkinnerJoinTranslator(
    const plan::SkinnerJoinOperator& join, khir::ProgramBuilder& program,
    execution::PipelineBuilder& pipeline_builder, execution::QueryState& state,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(join, std::move(children)),
      join_(join),
      program_(&program),
      pipeline_builder_(pipeline_builder),
      state_(state),
      expr_translator_(*program_, *this),
      cache_(join_.Children().size()) {}

bool RecompilingSkinnerJoinTranslator::ShouldExecute(
    int pred, int table_idx, const absl::flat_hash_set<int>& available_tables) {
  if (!tables_per_condition_[pred].contains(table_idx)) {
    return false;
  }

  for (int t : tables_per_condition_[pred]) {
    if (t == table_idx) continue;
    if (available_tables.contains(t)) continue;

    return false;
  }

  return true;
}

RecompilingJoinTranslator::ExecuteJoinFn
RecompilingSkinnerJoinTranslator::CompileJoinOrder(
    const std::vector<int>& order, void** materialized_buffers_raw,
    void** materialized_indexes_raw, void* tuple_idx_table_ptr_raw,
    int32_t* progress_arr_raw, int32_t* table_ctr_raw, int32_t* idx_arr_raw,
    int32_t* offset_arr_raw,
    std::add_pointer<int32_t(int32_t, int8_t)>::type valid_tuple_handler) {
  std::vector<std::string> func_names(order.size());
  std::string func_name = std::to_string(order[0]);
  for (int i = 1; i < order.size(); i++) {
    func_name += "-" + std::to_string(order[i]);
  }
  for (int i = 0; i < order.size(); i++) {
    func_names[order[i]] =
        "recompile_" + func_name + "_table_" + std::to_string(order[i]);
  }

  auto child_translators = this->Children();
  auto child_operators = this->join_.Children();
  auto conditions = join_.Conditions();

  auto& entry = cache_.GetOrInsert(order);
  if (entry.IsCompiled()) {
    return reinterpret_cast<ExecuteJoinFn>(entry.Func());
  }

  khir::ProgramBuilder program_builder;
  program_ = &program_builder;

  ForwardDeclare(*program_);
  ExpressionTranslator expr_translator(*program_, *this);

  // initially fill the child_translators schema values with garbage
  // this will get overwritten when we actually load tuples/update predicate
  // struct
  for (int i = 0; i < child_translators.size(); i++) {
    auto& child_translator = child_translators[i].get();
    auto& child_operator = child_operators[i].get();
    child_translator.SchemaValues().PopulateWithNotNull(
        *program_, child_operator.Schema());
  }

  // Setup struct of predicate columns
  // - 1 for the equality columns
  // - add remaining for each general column
  auto predicate_struct = std::make_unique<proxy::StructBuilder>(*program_);
  absl::flat_hash_map<std::pair<int, int>, int>
      colref_to_predicate_struct_field_;
  int pred_struct_size = 0;
  for (auto& c : predicate_columns_) {
    auto child_idx = c.get().GetChildIdx();
    auto column_idx = c.get().GetColumnIdx();
    colref_to_predicate_struct_field_[{child_idx, column_idx}] =
        pred_struct_size++;
    const auto& column_schema =
        child_operators[child_idx].get().Schema().Columns()[column_idx];
    predicate_struct->Add(column_schema.Expr().Type(),
                          column_schema.Expr().Nullable());
  }
  predicate_struct->Build();

  proxy::Struct global_predicate_struct(
      *program_, *predicate_struct,
      program_->Global(
          predicate_struct->Type(),
          program_->ConstantStruct(predicate_struct->Type(),
                                   predicate_struct->DefaultValues())));

  // Setup idx array.
  auto idx_array_type = program_->PointerType(program_->I32Type());
  auto idx_array =
      program_->PointerCast(program_->ConstPtr(idx_arr_raw), idx_array_type);

  // Setup progress array
  auto progress_array_type = program_->PointerType(program_->I32Type());
  auto progress_arr = program_->PointerCast(
      program_->ConstPtr(progress_arr_raw), progress_array_type);

  // Setup offset array
  auto offset_array_type = program_->PointerType(program_->I32Type());
  auto offset_array = program_->PointerCast(program_->ConstPtr(offset_arr_raw),
                                            offset_array_type);

  // Setup table_ctr
  auto table_ctr_type = program_->PointerType(program_->I32Type());
  auto table_ctr_ptr =
      program_->PointerCast(program_->ConstPtr(table_ctr_raw), table_ctr_type);

  // Regenerate all child struct types/buffers in new program
  std::vector<std::unique_ptr<proxy::MaterializedBuffer>> materialized_buffers;
  for (int i = 0; i < child_operators.size(); i++) {
    materialized_buffers.push_back(materialized_buffers_[i]->Regenerate(
        *program_, materialized_buffers_raw[i]));
  }

  // Regenerate all indexes in new program
  std::vector<std::unique_ptr<proxy::ColumnIndex>> indexes;
  for (int i = 0; i < indexes_.size(); i++) {
    indexes.push_back(
        indexes_[i]->Regenerate(*program_, materialized_indexes_raw[i]));
  }

  // Regenerate tuple idx table
  proxy::TupleIdxTable tuple_idx_table(
      *program_,
      program_->PointerCast(program_->ConstPtr(tuple_idx_table_ptr_raw),
                            program_->PointerType(program_->GetOpaqueType(
                                proxy::TupleIdxTable::TypeName))));

  std::vector<RecompilingTableFunction> table_functions;

  absl::flat_hash_set<int> available_tables;
  for (int t : order) {
    available_tables.insert(t);
  }

  for (int order_idx = order.size() - 1; order_idx >= 0; order_idx--) {
    auto table_idx = order[order_idx];

    available_tables.erase(table_idx);
    int bucket_list_max_size = predicate_columns_.size();

    auto handler_ptr =
        table_functions.empty() ? nullptr : &table_functions.back();

    table_functions.emplace_back(
        func_names[table_idx], *program_,
        [&](auto& initial_budget, auto& resume_progress) {
          absl::flat_hash_set<std::pair<int, int>> loaded_columns;
          auto& buffer = *materialized_buffers[table_idx];
          auto cardinality = buffer.Size();

          // for each equality predicate, if the buffer DNE, then return since
          // no result tuples with current column values.
          proxy::ColumnIndexBucketArray bucket_list(*program_,
                                                    bucket_list_max_size);

          absl::flat_hash_set<int> executed;
          for (auto pred : conditions_per_table_[table_idx]) {
            if (!ShouldExecute(pred, table_idx, available_tables)) {
              continue;
            }

            auto cond = conditions[pred];
            if (!EqualityPredicate(cond)) {
              continue;
            }

            executed.insert(pred);

            const auto& eq =
                dynamic_cast<const plan::BinaryArithmeticExpression&>(
                    cond.get());
            const auto& left =
                dynamic_cast<const plan::ColumnRefExpression&>(eq.LeftChild());
            const auto& right =
                dynamic_cast<const plan::ColumnRefExpression&>(eq.RightChild());

            // load if loaded
            const auto& table_colref =
                left.GetChildIdx() == table_idx ? left : right;
            auto table_column = table_colref.GetColumnIdx();
            const auto& other_colref =
                left.GetChildIdx() == table_idx ? right : left;

            std::pair<int, int> key = {other_colref.GetChildIdx(),
                                       other_colref.GetColumnIdx()};
            if (!loaded_columns.contains(key)) {
              auto field = colref_to_predicate_struct_field_.at(key);
              auto& child_translator = child_translators[key.first].get();
              child_translator.SchemaValues().SetValue(
                  key.second, global_predicate_struct.Get(field));
              loaded_columns.insert(key);
            }

            auto other_side_value = expr_translator.Compute(other_colref);

            proxy::If(
                *program_, other_side_value.IsNull(),
                [&]() {
                  auto budget = initial_budget - 1;
                  proxy::If(
                      *program_, budget == 0,
                      [&]() {
                        auto idx_ptr = program_->StaticGEP(
                            idx_array_type, idx_array, {0, table_idx});
                        program_->StoreI32(idx_ptr, (cardinality - 1).Get());
                        program_->StoreI32(
                            program_->StaticGEP(table_ctr_type, table_ctr_ptr,
                                                {0, 0}),
                            program_->ConstI32(table_idx));
                        program_->Return(program_->ConstI32(-1));
                      },
                      [&]() { program_->Return(budget.Get()); });
                },
                [&]() {
                  auto it =
                      column_to_index_idx_.find({table_idx, table_column});
                  if (it == column_to_index_idx_.end()) {
                    throw std::runtime_error("Expected index.");
                  }
                  auto index_idx = it->second;

                  auto bucket_from_index =
                      indexes[index_idx]->GetBucket(other_side_value.Get());
                  bucket_list.PushBack(bucket_from_index);
                  proxy::If(*program_, bucket_from_index.DoesNotExist(), [&]() {
                    auto budget = initial_budget - 1;
                    proxy::If(
                        *program_, budget == 0,
                        [&]() {
                          auto idx_ptr = program_->StaticGEP(
                              idx_array_type, idx_array, {0, table_idx});
                          program_->StoreI32(idx_ptr, (cardinality - 1).Get());
                          program_->StoreI32(
                              program_->StaticGEP(table_ctr_type, table_ctr_ptr,
                                                  {0, 0}),
                              program_->ConstI32(table_idx));
                          program_->Return(program_->ConstI32(-1));
                        },
                        [&]() { program_->Return(budget.Get()); });
                  });
                });
          }

          if (!executed.empty()) {
            auto progress_next_tuple = proxy::Ternary(
                *program_, resume_progress,
                [&]() {
                  auto progress_ptr = program_->StaticGEP(
                      progress_array_type, progress_arr, {0, table_idx});
                  return proxy::Int32(*program_,
                                      program_->LoadI32(progress_ptr));
                },
                [&]() { return proxy::Int32(*program_, 0); });
            auto offset_next_tuple =
                proxy::Int32(*program_, program_->LoadI32(program_->StaticGEP(
                                            offset_array_type, offset_array,
                                            {0, table_idx}))) +
                1;
            auto initial_next_tuple = proxy::Ternary(
                *program_, offset_next_tuple > progress_next_tuple,
                [&]() { return offset_next_tuple; },
                [&]() { return progress_next_tuple; });

            bucket_list.InitSortedIntersection(initial_next_tuple);

            int result_max_size = 64;
            auto result_array_type =
                program_->ArrayType(program_->I32Type(), result_max_size);
            std::vector<khir::Value> initial_result_values(
                result_max_size, program_->ConstI32(0));
            auto result_array =
                program_->Global(result_array_type,
                                 program_->ConstantArray(
                                     result_array_type, initial_result_values));
            auto result =
                program_->StaticGEP(result_array_type, result_array, {0, 0});
            auto result_initial_size =
                bucket_list.PopulateSortedIntersectionResult(result,
                                                             result_max_size);
            proxy::Loop loop(
                *program_,
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

                  // loop over all elements in result
                  proxy::Loop result_loop(
                      *program_,
                      [&](auto& loop) {
                        loop.AddLoopVariable(proxy::Int32(*program_, 0));
                        loop.AddLoopVariable(budget);

                        auto continue_resume_progress = proxy::Ternary(
                            *program_, resume_progress,
                            [&]() {
                              auto bucket_next_tuple_ptr = program_->StaticGEP(
                                  program_->I32Type(), result, {0});
                              auto initial_next_tuple = proxy::Int32(
                                  *program_,
                                  program_->LoadI32(bucket_next_tuple_ptr));
                              return initial_next_tuple == progress_next_tuple;
                            },
                            [&]() { return proxy::Bool(*program_, false); });
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
                            *program_, result, bucket_idx);

                        auto idx_ptr = program_->StaticGEP(
                            idx_array_type, idx_array, {0, table_idx});
                        program_->StoreI32(idx_ptr, next_tuple.Get());

                        /*
                        proxy::Printer printer(*program_, true);
                        printer.Print(proxy::Int32(*program_, table_idx));
                        printer.Print(next_tuple);
                        printer.PrintNewline();
                        */

                        // Store each of this table's predicate column
                        // values into the global_predicate_struct.
                        std::vector<std::pair<int, int>> cols;
                        for (auto [col_ref, field] :
                             colref_to_predicate_struct_field_) {
                          auto [child_idx, col_idx] = col_ref;
                          if (child_idx == table_idx) {
                            cols.emplace_back(col_idx, field);
                          }
                        }
                        std::sort(cols.begin(), cols.end(),
                                  [](const auto& p1, const auto& p2) {
                                    return p1.first < p2.first;
                                  });

                        for (auto [col_idx, field] : cols) {
                          auto value = buffer.Get(next_tuple, col_idx);
                          child_translators[table_idx]
                              .get()
                              .SchemaValues()
                              .SetValue(col_idx, value);
                        }

                        for (auto pred : conditions_per_table_[table_idx]) {
                          if (!ShouldExecute(pred, table_idx,
                                             available_tables)) {
                            continue;
                          }

                          if (executed.contains(pred)) {
                            continue;
                          }

                          for (const auto& col : columns_per_predicate_[pred]) {
                            std::pair<int, int> key = {
                                col.get().GetChildIdx(),
                                col.get().GetColumnIdx()};
                            if (key.first == table_idx) continue;
                            if (!loaded_columns.contains(key)) {
                              auto field =
                                  colref_to_predicate_struct_field_.at(key);
                              auto& child_translator =
                                  child_translators[key.first].get();
                              child_translator.SchemaValues().SetValue(
                                  key.second,
                                  global_predicate_struct.Get(field));
                              loaded_columns.insert(key);
                            }
                          }

                          auto cond =
                              expr_translator.Compute(conditions[pred].get());

                          proxy::If(
                              *program_, cond.IsNull(),
                              [&]() {
                                // If budget, depleted return -1 and set
                                // table ctr
                                proxy::If(
                                    *program_, budget == 0,
                                    [&]() {
                                      program_->StoreI32(
                                          program_->StaticGEP(table_ctr_type,
                                                              table_ctr_ptr,
                                                              {0, 0}),
                                          program_->ConstI32(table_idx));
                                      program_->Return(program_->ConstI32(-1));
                                    },
                                    [&]() {
                                      loop.Continue(
                                          bucket_idx + 1, budget,
                                          proxy::Bool(*program_, false));
                                    });
                              },
                              [&]() {
                                proxy::If(
                                    *program_, NOT,
                                    static_cast<proxy::Bool&>(cond.Get()),
                                    [&]() {
                                      // If budget, depleted return -1 and
                                      // set table ctr
                                      proxy::If(
                                          *program_, budget == 0,
                                          [&]() {
                                            program_->StoreI32(
                                                program_->StaticGEP(
                                                    table_ctr_type,
                                                    table_ctr_ptr, {0, 0}),
                                                program_->ConstI32(table_idx));
                                            program_->Return(
                                                program_->ConstI32(-1));
                                          },
                                          [&]() {
                                            loop.Continue(
                                                bucket_idx + 1, budget,
                                                proxy::Bool(*program_, false));
                                          });
                                    });
                              });
                        }

                        proxy::If(*program_, budget == 0, [&]() {
                          program_->StoreI32(
                              program_->StaticGEP(table_ctr_type, table_ctr_ptr,
                                                  {0, 0}),
                              program_->ConstI32(table_idx));
                          program_->Return(program_->ConstI32(-2));
                        });

                        for (auto [col_idx, field] : cols) {
                          auto value = child_translators[table_idx]
                                           .get()
                                           .SchemaValues()
                                           .Value(col_idx);
                          global_predicate_struct.Update(field, value);
                        }

                        proxy::Int32 next_budget(*program_, 0);
                        if (order_idx == order.size() - 1) {
                          auto handler = program_->PointerCast(
                              program_->ConstPtr((void*)valid_tuple_handler),
                              program_->PointerType(program_->FunctionType(
                                  program_->I32Type(),
                                  {program_->I32Type(), program_->I1Type()})));

                          next_budget = proxy::Int32(
                              *program_,
                              program_->Call(handler, {budget.Get(),
                                                       resume_progress.Get()}));
                        } else {
                          auto handler = handler_ptr->Get();
                          next_budget = proxy::Int32(
                              *program_,
                              program_->Call(handler, {budget.Get(),
                                                       resume_progress.Get()}));
                        }

                        // Valid tuple
                        proxy::If(*program_, next_budget < 0, [&]() {
                          program_->Return(next_budget.Get());
                        });

                        return loop.Continue(bucket_idx + 1, next_budget,
                                             proxy::Bool(*program_, false));
                      });

                  auto next_budget =
                      result_loop.template GetLoopVariable<proxy::Int32>(1);
                  auto next_result_size =
                      bucket_list.PopulateSortedIntersectionResult(
                          result, result_max_size);
                  return loop.Continue(next_budget, next_result_size);
                });

            return loop.template GetLoopVariable<proxy::Int32>(0);
          } else {
            // Loop over tuples in buffer
            proxy::Loop loop(
                *program_,
                [&](auto& loop) {
                  auto progress_next_tuple = proxy::Ternary(
                      *program_, resume_progress,
                      [&]() {
                        auto progress_ptr = program_->StaticGEP(
                            progress_array_type, progress_arr, {0, table_idx});
                        return proxy::Int32(*program_,
                                            program_->LoadI32(progress_ptr));
                      },
                      [&]() { return proxy::Int32(*program_, 0); });
                  auto offset_next_tuple =
                      proxy::Int32(*program_,
                                   program_->LoadI32(program_->StaticGEP(
                                       offset_array_type, offset_array,
                                       {0, table_idx}))) +
                      1;
                  auto initial_next_tuple = proxy::Ternary(
                      *program_, offset_next_tuple > progress_next_tuple,
                      [&]() { return offset_next_tuple; },
                      [&]() { return progress_next_tuple; });

                  loop.AddLoopVariable(initial_next_tuple);
                  loop.AddLoopVariable(initial_budget);

                  auto continue_resume_progress = proxy::Ternary(
                      *program_, resume_progress,
                      [&]() {
                        auto progress_ptr = program_->StaticGEP(
                            progress_array_type, progress_arr, {0, table_idx});
                        auto progress_next_tuple = proxy::Int32(
                            *program_, program_->LoadI32(progress_ptr));
                        return initial_next_tuple == progress_next_tuple;
                      },
                      [&]() { return proxy::Bool(*program_, false); });
                  loop.AddLoopVariable(continue_resume_progress);
                },
                [&](auto& loop) {
                  auto next_tuple =
                      loop.template GetLoopVariable<proxy::Int32>(0);
                  return next_tuple < cardinality;
                },
                [&](auto& loop) {
                  auto next_tuple =
                      loop.template GetLoopVariable<proxy::Int32>(0);
                  auto budget =
                      loop.template GetLoopVariable<proxy::Int32>(1) - 1;
                  auto resume_progress =
                      loop.template GetLoopVariable<proxy::Bool>(2);

                  auto idx_ptr = program_->StaticGEP(idx_array_type, idx_array,
                                                     {0, table_idx});
                  program_->StoreI32(idx_ptr, next_tuple.Get());

                  /*
                  proxy::Printer printer(*program_, true);
                  printer.Print(proxy::Int32(*program_, table_idx));
                  printer.Print(next_tuple);
                  printer.PrintNewline();
                  */

                  // Store each of this table's predicate column
                  // values into the global_predicate_struct.
                  std::vector<std::pair<int, int>> cols;
                  for (auto [col_ref, field] :
                       colref_to_predicate_struct_field_) {
                    auto [child_idx, col_idx] = col_ref;
                    if (child_idx == table_idx) {
                      cols.emplace_back(col_idx, field);
                    }
                  }
                  std::sort(cols.begin(), cols.end(),
                            [](const auto& p1, const auto& p2) {
                              return p1.first < p2.first;
                            });

                  for (auto [col_idx, field] : cols) {
                    auto value = buffer.Get(next_tuple, col_idx);
                    child_translators[table_idx].get().SchemaValues().SetValue(
                        col_idx, std::move(value));
                  }

                  for (auto pred : conditions_per_table_[table_idx]) {
                    if (!ShouldExecute(pred, table_idx, available_tables)) {
                      continue;
                    }

                    for (const auto& col : columns_per_predicate_[pred]) {
                      std::pair<int, int> key = {col.get().GetChildIdx(),
                                                 col.get().GetColumnIdx()};
                      if (key.first == table_idx) continue;
                      if (!loaded_columns.contains(key)) {
                        auto field = colref_to_predicate_struct_field_.at(key);
                        auto& child_translator =
                            child_translators[key.first].get();
                        child_translator.SchemaValues().SetValue(
                            key.second, global_predicate_struct.Get(field));
                        loaded_columns.insert(key);
                      }
                    }

                    auto cond = expr_translator.Compute(conditions[pred].get());

                    proxy::If(
                        *program_, cond.IsNull(),
                        [&]() {
                          // If budget, depleted return -1 and set
                          // table ctr
                          proxy::If(
                              *program_, budget == 0,
                              [&]() {
                                program_->StoreI32(
                                    program_->StaticGEP(table_ctr_type,
                                                        table_ctr_ptr, {0, 0}),
                                    program_->ConstI32(table_idx));
                                program_->Return(program_->ConstI32(-1));
                              },
                              [&]() {
                                loop.Continue(next_tuple + 1, budget,
                                              proxy::Bool(*program_, false));
                              });
                        },
                        [&]() {
                          proxy::If(
                              *program_, NOT,
                              static_cast<proxy::Bool&>(cond.Get()), [&]() {
                                // If budget, depleted return -1 and set
                                // table ctr
                                proxy::If(
                                    *program_, budget == 0,
                                    [&]() {
                                      program_->StoreI32(
                                          program_->StaticGEP(table_ctr_type,
                                                              table_ctr_ptr,
                                                              {0, 0}),
                                          program_->ConstI32(table_idx));
                                      program_->Return(program_->ConstI32(-1));
                                    },
                                    [&]() {
                                      loop.Continue(
                                          next_tuple + 1, budget,
                                          proxy::Bool(*program_, false));
                                    });
                              });
                        });
                  }

                  proxy::If(*program_, budget == 0, [&]() {
                    program_->StoreI32(
                        program_->StaticGEP(table_ctr_type, table_ctr_ptr,
                                            {0, 0}),
                        program_->ConstI32(table_idx));
                    program_->Return(program_->ConstI32(-2));
                  });

                  for (auto [col_idx, field] : cols) {
                    auto value =
                        child_translators[table_idx].get().SchemaValues().Value(
                            col_idx);
                    global_predicate_struct.Update(field, value);
                  }

                  // Valid tuple
                  proxy::Int32 next_budget(*program_, 0);
                  if (order_idx == order.size() - 1) {
                    auto handler = program_->PointerCast(
                        program_->ConstPtr((void*)valid_tuple_handler),
                        program_->PointerType(program_->FunctionType(
                            program_->I32Type(),
                            {program_->I32Type(), program_->I1Type()})));

                    next_budget = proxy::Int32(
                        *program_,
                        program_->Call(handler,
                                       {budget.Get(), resume_progress.Get()}));
                  } else {
                    auto handler = handler_ptr->Get();
                    next_budget = proxy::Int32(
                        *program_,
                        program_->Call(handler,
                                       {budget.Get(), resume_progress.Get()}));
                  }

                  proxy::If(*program_, next_budget < 0,
                            [&]() { program_->Return(next_budget.Get()); });

                  return loop.Continue(next_tuple + 1, next_budget,
                                       proxy::Bool(*program_, false));
                });

            return loop.template GetLoopVariable<proxy::Int32>(1);
          }
        });
  }

  entry.Compile(program_builder.Build(), func_names[order[0]]);
  return reinterpret_cast<ExecuteJoinFn>(entry.Func());
}

void RecompilingSkinnerJoinTranslator::Produce(proxy::Pipeline& output) {
  auto child_translators = this->Children();
  auto child_operators = this->join_.Children();
  auto conditions = join_.Conditions();

  int index_idx = 0;
  for (int i = 0; i < conditions.size(); i++) {
    PredicateColumnCollector collector;
    conditions[i].get().Accept(collector);
    columns_per_predicate_.push_back(collector.PredicateColumns());

    for (const auto& col_ref : columns_per_predicate_.back()) {
      std::pair<int, int> key = {col_ref.get().GetChildIdx(),
                                 col_ref.get().GetColumnIdx()};

      if (!column_to_index_idx_.contains(key)) {
        if (join_.PrefixOrder().empty() ||
            join_.PrefixOrder()[0] != key.first) {
          column_to_index_idx_[key] = index_idx++;
        }

        predicate_columns_.push_back(col_ref);
      }
    }
  }
  indexes_ = std::vector<std::unique_ptr<proxy::ColumnIndex>>(index_idx);

  // 1. Materialize each child.
  for (int i = 0; i < child_translators.size(); i++) {
    auto& child_translator = child_translators[i].get();
    auto& child_operator = child_operators[i].get();

    if (auto scan = dynamic_cast<ScanTranslator*>(&child_translator)) {
      auto disk_materialized_buffer = scan->GenerateBuffer();
      std::vector<int> indexes;
      for (const auto& [key, value] : column_to_index_idx_) {
        auto [child_idx, col_idx] = key;
        auto index_idx = value;
        if (i != child_idx) {
          continue;
        }
        indexes.push_back(index_idx);

        if (scan->HasIndex(col_idx)) {
          indexes_[index_idx] = scan->GenerateIndex(col_idx);
        } else {
          throw std::runtime_error("TODO add scan + memory index");
        }
      }

      proxy::Pipeline input(*program_, pipeline_builder_);
      input.Init([&]() {
        disk_materialized_buffer->Init();
        for (int index_idx : indexes) {
          indexes_[index_idx]->Init();
        }
      });
      input.Reset([&]() {
        disk_materialized_buffer->Reset();
        for (int index_idx : indexes) {
          indexes_[index_idx]->Reset();
        }
      });
      input.Build();
      output.Get().AddPredecessor(input.Get());
      materialized_buffers_.push_back(std::move(disk_materialized_buffer));
    } else {
      proxy::Pipeline input(*program_, pipeline_builder_);

      // For each predicate column on this table, declare a memory index.
      std::vector<int> indexes;
      for (const auto& [key, value] : column_to_index_idx_) {
        auto [child_idx, col_idx] = key;
        auto index_idx = value;
        if (i != child_idx) {
          continue;
        }

        auto type = child_operator.Schema().Columns()[col_idx].Expr().Type();
        indexes_[index_idx] =
            std::make_unique<proxy::MemoryColumnIndex>(*program_, state_, type);
        indexes.push_back(index_idx);
      }

      // Create struct for materialization
      auto struct_builder = std::make_unique<proxy::StructBuilder>(*program_);
      const auto& child_schema = child_operator.Schema().Columns();
      for (const auto& col : child_schema) {
        struct_builder->Add(col.Expr().Type(), col.Expr().Nullable());
      }
      struct_builder->Build();
      proxy::Vector buffer(*program_, state_, *struct_builder);

      input.Init([&]() {
        buffer.Init();
        for (int index_idx : indexes) {
          indexes_[index_idx]->Init();
        }
      });

      input.Reset([&]() {
        buffer.Reset();
        for (int index_idx : indexes) {
          indexes_[index_idx]->Reset();
        }
      });

      // Fill buffer/indexes
      child_idx_ = i;
      buffer_ = &buffer;
      child_translator.Produce(input);
      buffer_ = nullptr;

      input.Build();
      output.Get().AddPredecessor(input.Get());

      materialized_buffers_.push_back(
          std::make_unique<proxy::MemoryMaterializedBuffer>(
              *program_, std::move(struct_builder), std::move(buffer)));
    }
  }

  // 2. Setup join evaluation
  // Setup global hash table that contains tuple idx
  proxy::TupleIdxTable tuple_idx_table(*program_);

  // Setup # of result_tuples
  auto num_result_tuples_type = program_->ArrayType(program_->I32Type(), 1);
  auto num_result_tuples_ptr = program_->Global(
      num_result_tuples_type,
      program_->ConstantArray(num_result_tuples_type, {program_->ConstI32(0)}));

  // Setup idx array.
  std::vector<khir::Value> initial_idx_values(child_operators.size(),
                                              program_->ConstI32(0));
  auto idx_array_type =
      program_->ArrayType(program_->I32Type(), child_operators.size());
  auto idx_array = program_->Global(
      idx_array_type,
      program_->ConstantArray(idx_array_type, initial_idx_values));

  // Setup function for each valid tuple
  static int valid_id = 0;
  RecompilingTableFunction valid_tuple_handler(
      "recompile_valid_tuple_handler" + std::to_string(valid_id++), *program_,
      [&](const auto& budget, const auto& resume_progress) {
        // Insert tuple idx into hash table
        auto tuple_idx_arr =
            program_->StaticGEP(idx_array_type, idx_array, {0, 0});

        proxy::Int32 num_tables(*program_, child_translators.size());

        proxy::If(
            *program_, tuple_idx_table.Insert(tuple_idx_arr, num_tables), [&] {
              std::vector<absl::flat_hash_set<int>> output_columns(
                  child_translators.size());
              for (const auto& column : join_.Schema().Columns()) {
                PredicateColumnCollector collector;
                column.Expr().Accept(collector);
                for (auto col : collector.PredicateColumns()) {
                  auto child_idx = col.get().GetChildIdx();
                  auto col_idx = col.get().GetColumnIdx();
                  output_columns[child_idx].insert(col_idx);
                }
              }

              for (int i = 0; i < child_translators.size(); i++) {
                auto& child_translator = child_translators[i].get();
                const auto& schema = child_operators[i].get().Schema();
                child_translator.SchemaValues().PopulateWithNotNull(*program_,
                                                                    schema);
              }

              int current_buffer = 0;
              for (int i = 0; i < child_translators.size(); i++) {
                auto& child_translator = child_translators[i].get();

                auto tuple_idx_ptr = program_->StaticGEP(program_->I32Type(),
                                                         tuple_idx_arr, {i});
                auto tuple_idx =
                    proxy::Int32(*program_, program_->LoadI32(tuple_idx_ptr));

                auto& buffer = *materialized_buffers_[current_buffer++];

                if (output_columns[i].empty()) continue;

                // set the schema values of child to be the tuple_idx'th tuple
                // of current table.
                if (auto buf = dynamic_cast<proxy::MemoryMaterializedBuffer*>(
                        &buffer)) {
                  child_translator.SchemaValues().SetValues(buffer[tuple_idx]);
                } else {
                  for (int col : output_columns[i]) {
                    child_translator.SchemaValues().SetValue(
                        col, buffer.Get(tuple_idx, col));
                  }
                }
              }

              // Compute the output schema
              this->values_.ResetValues();
              for (const auto& column : join_.Schema().Columns()) {
                this->values_.AddVariable(
                    expr_translator_.Compute(column.Expr()));
              }

              // Push tuples to next operator
              if (auto parent = this->Parent()) {
                parent->get().Consume(*this);
              }
            });

        auto result_ptr = program_->StaticGEP(num_result_tuples_type,
                                              num_result_tuples_ptr, {0, 0});
        proxy::Int32 num_result_tuples(*program_,
                                       program_->LoadI32(result_ptr));
        program_->StoreI32(result_ptr, (num_result_tuples + 1).Get());

        return budget;
      });

  // pass all materialized buffers to the executor
  auto materialized_buffer_array_type = program_->ArrayType(
      program_->PointerType(program_->I8Type()), materialized_buffers_.size());
  auto materialized_buffer_array_init = program_->ConstantArray(
      materialized_buffer_array_type,
      std::vector<khir::Value>(
          materialized_buffers_.size(),
          program_->NullPtr(program_->PointerType(program_->I8Type()))));
  auto materialized_buffer_array = program_->Global(
      materialized_buffer_array_type, materialized_buffer_array_init);

  // pass all materialized indexes to the executor
  auto materialized_index_array_type = program_->ArrayType(
      program_->PointerType(program_->I8Type()), indexes_.size());
  auto materialized_index_array_init = program_->ConstantArray(
      materialized_index_array_type,
      std::vector<khir::Value>(
          indexes_.size(),
          program_->NullPtr(program_->PointerType(program_->I8Type()))));
  auto materialized_index_array = program_->Global(
      materialized_index_array_type, materialized_index_array_init);

  // pass all cardinalities to the executor
  auto cardinalities_array_type =
      program_->ArrayType(program_->I32Type(), materialized_buffers_.size());
  auto cardinalities_array_init =
      program_->ConstantArray(cardinalities_array_type,
                              std::vector<khir::Value>(child_translators.size(),
                                                       program_->ConstI32(0)));
  auto cardinalities_array =
      program_->Global(cardinalities_array_type, cardinalities_array_init);

  output.Body([&]() {
    // 3. Execute join
    // Initialize tuple idx table
    tuple_idx_table.Init();

    for (int i = 0; i < materialized_buffers_.size(); i++) {
      program_->StorePtr(program_->StaticGEP(materialized_buffer_array_type,
                                             materialized_buffer_array, {0, i}),
                         materialized_buffers_[i]->Serialize());
    }

    for (int i = 0; i < indexes_.size(); i++) {
      program_->StorePtr(program_->StaticGEP(materialized_index_array_type,
                                             materialized_index_array, {0, i}),
                         indexes_[i]->Serialize());
    }

    for (int i = 0; i < materialized_buffers_.size(); i++) {
      program_->StoreI32(program_->StaticGEP(cardinalities_array_type,
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
    auto compile_fn = static_cast<RecompilingJoinTranslator*>(this);
    proxy::SkinnerJoinExecutor::ExecuteRecompilingJoin(
        *program_, child_translators.size(),
        program_->StaticGEP(cardinalities_array_type, cardinalities_array,
                            {0, 0}),
        &table_connections_, &join_.PrefixOrder(), compile_fn,
        program_->StaticGEP(materialized_buffer_array_type,
                            materialized_buffer_array, {0, 0}),
        program_->StaticGEP(materialized_index_array_type,
                            materialized_index_array, {0, 0}),
        tuple_idx_table.Get(),
        program_->StaticGEP(idx_array_type, idx_array, {0, 0}),
        program_->StaticGEP(num_result_tuples_type, num_result_tuples_ptr,
                            {0, 0}),
        program_->GetFunctionPointer(valid_tuple_handler.Get()));

    tuple_idx_table.Reset();
  });
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
      proxy::If(*program_, NOT, value.IsNull(), [&]() {
        dynamic_cast<proxy::ColumnIndexBuilder*>(indexes_[index_idx].get())
            ->Insert(value.Get(), tuple_idx);
      });
    }
  }

  // insert tuple into buffer
  buffer_->PushBack().Pack(values);
}

}  // namespace kush::compile