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

class RecompilingTableFunction {
 public:
  RecompilingTableFunction(
      std::string name, khir::ProgramBuilder& program,
      std::function<proxy::Int32(proxy::Int32&, proxy::Bool&)> body) {
    func_ = program.CreatePublicFunction(
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
    execution::PipelineBuilder& pipeline_builder,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(join, std::move(children)),
      join_(join),
      program_(program),
      pipeline_builder_(pipeline_builder),
      expr_translator_(program_, *this),
      cache_(join_.Children().size()) {}

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
    int32_t* offset_arr_raw, int32_t* num_result_tuples_raw) {
  std::vector<std::string> func_names(order.size());
  std::string func_name = std::to_string(order[0]);
  for (int i = 1; i < order.size(); i++) {
    func_name += "-" + std::to_string(order[i]);
  }
  for (int i = 0; i < order.size(); i++) {
    func_names[order[i]] =
        "recompile_" + func_name + "_table_" + std::to_string(order[i]);
  }
  auto valid_tuple_handler_func_name =
      "recompile_" + func_name + "_valid_tuple_handler";

  auto child_translators = this->Children();
  auto child_operators = this->join_.Children();
  auto conditions = join_.Conditions();

  auto& entry = cache_.GetOrInsert(order);
  if (entry.IsCompiled()) {
    return reinterpret_cast<ExecuteJoinFn>(entry.Func(func_names[order[0]]));
  }

  auto& program = entry.ProgramBuilder();
  ForwardDeclare(program);
  ExpressionTranslator expr_translator(program, *this);

  // initially fill the child_translators schema values with garbage
  // this will get overwritten when we actually load tuples/update predicate
  // struct
  for (int i = 0; i < child_translators.size(); i++) {
    auto& child_translator = child_translators[i].get();
    auto& child_operator = child_operators[i].get();

    const auto& schema = child_operator.Schema().Columns();

    child_translator.SchemaValues().ResetValues();
    for (int i = 0; i < schema.size(); i++) {
      switch (schema[i].Expr().Type()) {
        case catalog::SqlType::SMALLINT:
          child_translator.SchemaValues().AddVariable(proxy::SQLValue(
              proxy::Int16(program, 0), proxy::Bool(program, false)));
          break;
        case catalog::SqlType::INT:
          child_translator.SchemaValues().AddVariable(proxy::SQLValue(
              proxy::Int32(program, 0), proxy::Bool(program, false)));
          break;
        case catalog::SqlType::DATE:
          child_translator.SchemaValues().AddVariable(
              proxy::SQLValue(proxy::Date(program, absl::CivilDay(2000, 1, 1)),
                              proxy::Bool(program, false)));
          break;
        case catalog::SqlType::BIGINT:
          child_translator.SchemaValues().AddVariable(proxy::SQLValue(
              proxy::Int64(program, 0), proxy::Bool(program, false)));
          break;
        case catalog::SqlType::BOOLEAN:
          child_translator.SchemaValues().AddVariable(proxy::SQLValue(
              proxy::Bool(program, false), proxy::Bool(program, false)));
          break;
        case catalog::SqlType::REAL:
          child_translator.SchemaValues().AddVariable(proxy::SQLValue(
              proxy::Float64(program, 0), proxy::Bool(program, false)));
          break;
        case catalog::SqlType::TEXT:
          child_translator.SchemaValues().AddVariable(proxy::SQLValue(
              proxy::String::Global(program, ""), proxy::Bool(program, false)));
          break;
      }
    }
  }

  // Setup struct of predicate columns
  // - 1 for the equality columns
  // - add remaining for each general column
  auto predicate_struct = std::make_unique<proxy::StructBuilder>(program);
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
      program, *predicate_struct,
      program.Global(
          false, true, predicate_struct->Type(),
          program.ConstantStruct(predicate_struct->Type(),
                                 predicate_struct->DefaultValues())));

  // Setup idx array.
  auto idx_array_type = program.PointerType(program.I32Type());
  auto idx_array =
      program.PointerCast(program.ConstPtr(idx_arr_raw), idx_array_type);

  // Setup progress array
  auto progress_array_type = program.PointerType(program.I32Type());
  auto progress_arr = program.PointerCast(program.ConstPtr(progress_arr_raw),
                                          progress_array_type);

  // Setup offset array
  auto offset_array_type = program.PointerType(program.I32Type());
  auto offset_array =
      program.PointerCast(program.ConstPtr(offset_arr_raw), offset_array_type);

  // Setup table_ctr
  auto table_ctr_type = program.PointerType(program.I32Type());
  auto table_ctr_ptr =
      program.PointerCast(program.ConstPtr(table_ctr_raw), table_ctr_type);

  // Setup # of result_tuples
  auto num_result_tuples_type = program.PointerType(program.I32Type());
  auto num_result_tuples_ptr = program.PointerCast(
      program.ConstPtr(num_result_tuples_raw), num_result_tuples_type);

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

  // Regenerate tuple idx table
  proxy::TupleIdxTable tuple_idx_table(
      program, program.PointerCast(program.ConstPtr(tuple_idx_table_ptr_raw),
                                   program.PointerType(program.GetOpaqueType(
                                       proxy::TupleIdxTable::TypeName))));

  std::vector<RecompilingTableFunction> table_functions;

  // Valid Tuple Handler
  table_functions.emplace_back(
      valid_tuple_handler_func_name, program,
      [&](const auto& budget, const auto& resume_progress) {
        // Insert tuple idx into hash table
        auto tuple_idx_arr =
            program.ConstGEP(idx_array_type, idx_array, {0, 0});

        proxy::Int32 num_tables(program, child_translators.size());
        tuple_idx_table.Insert(tuple_idx_arr, num_tables);

        auto result_ptr = program.ConstGEP(num_result_tuples_type,
                                           num_result_tuples_ptr, {0, 0});
        proxy::Int32 num_result_tuples(program, program.LoadI32(result_ptr));
        program.StoreI32(result_ptr, (num_result_tuples + 1).Get());

        return budget;
      });

  absl::flat_hash_set<int> available_tables;
  for (int t : order) {
    available_tables.insert(t);
  }

  for (int order_idx = order.size() - 1; order_idx >= 0; order_idx--) {
    auto table_idx = order[order_idx];
    auto handler = table_functions.back().Get();

    available_tables.erase(table_idx);
    int bucket_list_max_size = predicate_columns_.size();

    table_functions.emplace_back(
        func_names[table_idx], program,
        [&](auto& initial_budget, auto& resume_progress) {
          {
            // Unpack the predicate struct.
            auto column_values = global_predicate_struct.Unpack();

            for (const auto& [colref, field] :
                 colref_to_predicate_struct_field_) {
              auto [child_idx, col_idx] = colref;

              // Set the ColumnIdx value of the ChildIdx operator to be the
              // unpacked value.
              auto& child_translator = child_translators[child_idx].get();
              child_translator.SchemaValues().SetValue(col_idx,
                                                       column_values[field]);
            }
          }

          auto& buffer = *materialized_buffers[table_idx];
          auto cardinality = buffer.Size();

          // for each equality predicate, if the buffer DNE, then return since
          // no result tuples with current column values.
          proxy::ColumnIndexBucketArray bucket_list(program,
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

            auto other_side_value = left.GetChildIdx() == table_idx
                                        ? expr_translator.Compute(right)
                                        : expr_translator.Compute(left);
            auto table_column = left.GetChildIdx() == table_idx
                                    ? left.GetColumnIdx()
                                    : right.GetColumnIdx();

            proxy::If(
                program, other_side_value.IsNull(),
                [&]() {
                  auto budget = initial_budget - 1;
                  proxy::If(
                      program, budget == 0,
                      [&]() {
                        auto idx_ptr = program.ConstGEP(
                            idx_array_type, idx_array, {0, table_idx});
                        program.StoreI32(idx_ptr, (cardinality - 1).Get());
                        program.StoreI32(
                            program.ConstGEP(table_ctr_type, table_ctr_ptr,
                                             {0, 0}),
                            program.ConstI32(table_idx));
                        program.Return(program.ConstI32(-1));
                      },
                      [&]() { program.Return(budget.Get()); });
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
                  proxy::If(program, bucket_from_index.DoesNotExist(), [&]() {
                    auto budget = initial_budget - 1;
                    proxy::If(
                        program, budget == 0,
                        [&]() {
                          auto idx_ptr = program.ConstGEP(
                              idx_array_type, idx_array, {0, table_idx});
                          program.StoreI32(idx_ptr, (cardinality - 1).Get());
                          program.StoreI32(
                              program.ConstGEP(table_ctr_type, table_ctr_ptr,
                                               {0, 0}),
                              program.ConstI32(table_idx));
                          program.Return(program.ConstI32(-1));
                        },
                        [&]() { program.Return(budget.Get()); });
                  });
                });
          }

          if (!executed.empty()) {
            auto progress_next_tuple = proxy::Ternary(
                program, resume_progress,
                [&]() {
                  auto progress_ptr = program.ConstGEP(
                      progress_array_type, progress_arr, {0, table_idx});
                  return proxy::Int32(program, program.LoadI32(progress_ptr));
                },
                [&]() { return proxy::Int32(program, 0); });
            auto offset_next_tuple =
                proxy::Int32(program, program.LoadI32(program.ConstGEP(
                                          offset_array_type, offset_array,
                                          {0, table_idx}))) +
                1;
            auto initial_next_tuple = proxy::Ternary(
                program, offset_next_tuple > progress_next_tuple,
                [&]() { return offset_next_tuple; },
                [&]() { return progress_next_tuple; });

            bucket_list.InitSortedIntersection(initial_next_tuple);

            int result_max_size = 64;
            auto result_array_type =
                program.ArrayType(program.I32Type(), result_max_size);
            std::vector<khir::Value> initial_result_values(result_max_size,
                                                           program.ConstI32(0));
            auto result_array =
                program.Global(false, true, result_array_type,
                               program.ConstantArray(result_array_type,
                                                     initial_result_values));
            auto result =
                program.ConstGEP(result_array_type, result_array, {0, 0});
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

                  // loop over all elements in result
                  proxy::Loop result_loop(
                      program,
                      [&](auto& loop) {
                        loop.AddLoopVariable(proxy::Int32(program, 0));
                        loop.AddLoopVariable(budget);

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

                        auto idx_ptr = program.ConstGEP(
                            idx_array_type, idx_array, {0, table_idx});
                        program.StoreI32(idx_ptr, next_tuple.Get());

                        /*
                        proxy::Printer printer(program, true);
                        printer.Print(proxy::Int32(program, table_idx));
                        printer.Print(next_tuple);
                        printer.PrintNewline();
                        */

                        auto current_table_values = buffer[next_tuple];

                        // Store each of this table's predicate column
                        // values into the global_predicate_struct.
                        for (auto [col_ref, field] :
                             colref_to_predicate_struct_field_) {
                          auto [child_idx, col_idx] = col_ref;
                          if (child_idx == table_idx) {
                            global_predicate_struct.Update(
                                field, current_table_values[col_idx]);

                            // Additionally, update this table's values to
                            // read from the unpacked tuple instead of the
                            // old loaded value from
                            // global_predicate_struct.
                            child_translators[table_idx]
                                .get()
                                .SchemaValues()
                                .SetValue(
                                    col_idx,
                                    std::move(current_table_values[col_idx]));
                          }
                        }

                        for (auto pred : conditions_per_table_[table_idx]) {
                          if (!ShouldExecute(pred, table_idx,
                                             available_tables)) {
                            continue;
                          }

                          if (executed.contains(pred)) {
                            continue;
                          }

                          auto cond =
                              expr_translator.Compute(conditions[pred].get());

                          proxy::If(
                              program, cond.IsNull(),
                              [&]() {
                                // If budget, depleted return -1 and set
                                // table ctr
                                proxy::If(
                                    program, budget == 0,
                                    [&]() {
                                      program.StoreI32(
                                          program.ConstGEP(table_ctr_type,
                                                           table_ctr_ptr,
                                                           {0, 0}),
                                          program.ConstI32(table_idx));
                                      program.Return(program.ConstI32(-1));
                                    },
                                    [&]() {
                                      loop.Continue(
                                          bucket_idx + 1, budget,
                                          proxy::Bool(program, false));
                                    });
                              },
                              [&]() {
                                proxy::If(
                                    program,
                                    !static_cast<proxy::Bool&>(cond.Get()),
                                    [&]() {
                                      // If budget, depleted return -1 and
                                      // set table ctr
                                      proxy::If(
                                          program, budget == 0,
                                          [&]() {
                                            program.StoreI32(
                                                program.ConstGEP(table_ctr_type,
                                                                 table_ctr_ptr,
                                                                 {0, 0}),
                                                program.ConstI32(table_idx));
                                            program.Return(
                                                program.ConstI32(-1));
                                          },
                                          [&]() {
                                            loop.Continue(
                                                bucket_idx + 1, budget,
                                                proxy::Bool(program, false));
                                          });
                                    });
                              });
                        }

                        proxy::If(program, budget == 0, [&]() {
                          program.StoreI32(
                              program.ConstGEP(table_ctr_type, table_ctr_ptr,
                                               {0, 0}),
                              program.ConstI32(table_idx));
                          program.Return(program.ConstI32(-2));
                        });

                        // Valid tuple
                        auto next_budget = proxy::Int32(
                            program,
                            program.Call(handler, {budget.Get(),
                                                   resume_progress.Get()}));
                        proxy::If(program, next_budget < 0,
                                  [&]() { program.Return(next_budget.Get()); });

                        return loop.Continue(bucket_idx + 1, next_budget,
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
          } else {
            // Loop over tuples in buffer
            proxy::Loop loop(
                program,
                [&](auto& loop) {
                  auto progress_next_tuple = proxy::Ternary(
                      program, resume_progress,
                      [&]() {
                        auto progress_ptr = program.ConstGEP(
                            progress_array_type, progress_arr, {0, table_idx});
                        return proxy::Int32(program,
                                            program.LoadI32(progress_ptr));
                      },
                      [&]() { return proxy::Int32(program, 0); });
                  auto offset_next_tuple =
                      proxy::Int32(program, program.LoadI32(program.ConstGEP(
                                                offset_array_type, offset_array,
                                                {0, table_idx}))) +
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
                        auto progress_ptr = program.ConstGEP(
                            progress_array_type, progress_arr, {0, table_idx});
                        auto progress_next_tuple = proxy::Int32(
                            program, program.LoadI32(progress_ptr));
                        return initial_next_tuple == progress_next_tuple;
                      },
                      [&]() { return proxy::Bool(program, false); });
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

                  auto idx_ptr = program.ConstGEP(idx_array_type, idx_array,
                                                  {0, table_idx});
                  program.StoreI32(idx_ptr, next_tuple.Get());

                  /*
                  proxy::Printer printer(program, true);
                  printer.Print(proxy::Int32(program, table_idx));
                  printer.Print(next_tuple);
                  printer.PrintNewline();
                  */

                  auto current_table_values = buffer[next_tuple];

                  // Store each of this table's predicate column
                  // values into the global_predicate_struct.
                  for (auto [col_ref, field] :
                       colref_to_predicate_struct_field_) {
                    auto [child_idx, col_idx] = col_ref;
                    if (child_idx == table_idx) {
                      global_predicate_struct.Update(
                          field, current_table_values[col_idx]);

                      // Additionally, update this table's values to
                      // read from the unpacked tuple instead of the
                      // old loaded value from
                      // global_predicate_struct.
                      child_translators[table_idx]
                          .get()
                          .SchemaValues()
                          .SetValue(col_idx,
                                    std::move(current_table_values[col_idx]));
                    }
                  }

                  for (auto pred : conditions_per_table_[table_idx]) {
                    if (!ShouldExecute(pred, table_idx, available_tables)) {
                      continue;
                    }

                    auto cond = expr_translator.Compute(conditions[pred].get());

                    proxy::If(
                        program, cond.IsNull(),
                        [&]() {
                          // If budget, depleted return -1 and set
                          // table ctr
                          proxy::If(
                              program, budget == 0,
                              [&]() {
                                program.StoreI32(
                                    program.ConstGEP(table_ctr_type,
                                                     table_ctr_ptr, {0, 0}),
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
                              !static_cast<proxy::Bool&>(cond.Get() /*cond*/),
                              [&]() {
                                // If budget, depleted return -1 and set
                                // table ctr
                                proxy::If(
                                    program, budget == 0,
                                    [&]() {
                                      program.StoreI32(
                                          program.ConstGEP(table_ctr_type,
                                                           table_ctr_ptr,
                                                           {0, 0}),
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

                  proxy::If(program, budget == 0, [&]() {
                    program.StoreI32(
                        program.ConstGEP(table_ctr_type, table_ctr_ptr, {0, 0}),
                        program.ConstI32(table_idx));
                    program.Return(program.ConstI32(-2));
                  });

                  // Valid tuple
                  auto next_budget = proxy::Int32(
                      program, program.Call(handler, {budget.Get(),
                                                      resume_progress.Get()}));
                  proxy::If(program, next_budget < 0,
                            [&]() { program.Return(next_budget.Get()); });

                  return loop.Continue(next_tuple + 1, next_budget,
                                       proxy::Bool(program, false));
                });

            return loop.template GetLoopVariable<proxy::Int32>(1);
          }
        });
  }

  entry.Compile();
  return reinterpret_cast<ExecuteJoinFn>(entry.Func(func_names[order[0]]));
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
        if (join_.PrefixOrder().empty() ||
            join_.PrefixOrder()[0] != key.first) {
          column_to_index_idx_[key] = index_idx++;
        }

        predicate_columns_.push_back(col_ref);
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
  auto compile_fn = static_cast<RecompilingJoinTranslator*>(this);
  proxy::SkinnerJoinExecutor::ExecuteRecompilingJoin(
      program_, child_translators.size(),
      program_.ConstGEP(cardinalities_array_type, cardinalities_array, {0, 0}),
      &table_connections_, &join_.PrefixOrder(), compile_fn,
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
    const auto& schema = child_operators[i].get().Schema().Columns();
    child_translator.SchemaValues().ResetValues();
    for (int i = 0; i < schema.size(); i++) {
      switch (schema[i].Expr().Type()) {
        case catalog::SqlType::SMALLINT:
          child_translator.SchemaValues().AddVariable(proxy::SQLValue(
              proxy::Int16(program_, 0), proxy::Bool(program_, false)));
          break;
        case catalog::SqlType::INT:
          child_translator.SchemaValues().AddVariable(proxy::SQLValue(
              proxy::Int32(program_, 0), proxy::Bool(program_, false)));
          break;
        case catalog::SqlType::DATE:
          child_translator.SchemaValues().AddVariable(
              proxy::SQLValue(proxy::Date(program_, absl::CivilDay(2000, 1, 1)),
                              proxy::Bool(program_, false)));
          break;
        case catalog::SqlType::BIGINT:
          child_translator.SchemaValues().AddVariable(proxy::SQLValue(
              proxy::Int64(program_, 0), proxy::Bool(program_, false)));
          break;
        case catalog::SqlType::BOOLEAN:
          child_translator.SchemaValues().AddVariable(proxy::SQLValue(
              proxy::Bool(program_, false), proxy::Bool(program_, false)));
          break;
        case catalog::SqlType::REAL:
          child_translator.SchemaValues().AddVariable(proxy::SQLValue(
              proxy::Float64(program_, 0), proxy::Bool(program_, false)));
          break;
        case catalog::SqlType::TEXT:
          child_translator.SchemaValues().AddVariable(
              proxy::SQLValue(proxy::String::Global(program_, ""),
                              proxy::Bool(program_, false)));
          break;
      }
    }
  }

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

      if (output_columns[i].empty()) continue;

      // set the schema values of child to be the tuple_idx'th tuple of
      // current table.
      if (auto buf = dynamic_cast<proxy::MemoryMaterializedBuffer*>(&buffer)) {
        child_translator.SchemaValues().SetValues(buffer[tuple_idx]);
      } else {
        for (int col : output_columns[i]) {
          child_translator.SchemaValues().SetValue(col,
                                                   buffer.Get(tuple_idx, col));
        }
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