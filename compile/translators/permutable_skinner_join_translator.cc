#include "compile/translators/permutable_skinner_join_translator.h"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_set.h"

#include "compile/proxy/control_flow/if.h"
#include "compile/proxy/control_flow/loop.h"
#include "compile/proxy/function.h"
#include "compile/proxy/skinner_join_executor.h"
#include "compile/proxy/struct.h"
#include "compile/proxy/tuple_idx_table.h"
#include "compile/proxy/value/ir_value.h"
#include "compile/proxy/vector.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "compile/translators/predicate_column_collector.h"
#include "compile/translators/scan_translator.h"
#include "khir/program_builder.h"
#include "util/vector_util.h"

namespace kush::compile {

PermutableSkinnerJoinTranslator::PermutableSkinnerJoinTranslator(
    const plan::SkinnerJoinOperator& join, khir::ProgramBuilder& program,
    execution::PipelineBuilder& pipeline_builder,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(join, std::move(children)),
      join_(join),
      program_(program),
      pipeline_builder_(pipeline_builder),
      expr_translator_(program_, *this) {}

bool IsEqualityPredicate(const kush::plan::Expression& predicate) {
  if (auto eq = dynamic_cast<const kush::plan::BinaryArithmeticExpression*>(
          &predicate)) {
    if (auto left_column = dynamic_cast<const kush::plan::ColumnRefExpression*>(
            &eq->LeftChild())) {
      if (auto right_column =
              dynamic_cast<const kush::plan::ColumnRefExpression*>(
                  &eq->RightChild())) {
        return true;
      }
    }
  }

  return false;
}

std::vector<int> IndexesAvailableForTable(
    int table_idx, std::vector<absl::btree_set<int>>& predicates_per_table,
    const std::vector<std::reference_wrapper<const kush::plan::Expression>>&
        conditions) {
  std::vector<int> result;
  for (int predicate_idx : predicates_per_table[table_idx]) {
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
          result.push_back(predicate_idx);
        }
      }
    }
  }

  return result;
}

void PermutableSkinnerJoinTranslator::Produce() {
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
  // Setup struct of predicate columns
  auto predicate_struct = std::make_unique<proxy::StructBuilder>(program_);
  for (auto& predicate_column : predicate_columns_) {
    const auto& child = child_operators[predicate_column.get().GetChildIdx()];
    const auto& col =
        child.get().Schema().Columns()[predicate_column.get().GetColumnIdx()];
    predicate_struct->Add(col.Expr().Type(), col.Expr().Nullable());
  }
  predicate_struct->Build();

  proxy::Struct global_predicate_struct(
      program_, *predicate_struct,
      program_.Global(
          false, true, predicate_struct->Type(),
          program_.ConstantStruct(predicate_struct->Type(),
                                  predicate_struct->DefaultValues())));

  // Setup idx array.
  std::vector<khir::Value> initial_idx_values(child_operators.size(),
                                              program_.ConstI32(0));
  auto idx_array_type =
      program_.ArrayType(program_.I32Type(), child_operators.size());
  auto idx_array = program_.Global(
      false, true, idx_array_type,
      program_.ConstantArray(idx_array_type, initial_idx_values));

  // Setup progress array
  std::vector<khir::Value> initial_progress_values(child_operators.size(),
                                                   program_.ConstI32(-1));
  auto progress_array_type =
      program_.ArrayType(program_.I32Type(), child_operators.size());
  auto progress_arr = program_.Global(
      false, true, progress_array_type,
      program_.ConstantArray(progress_array_type, initial_progress_values));

  // Setup offset array
  std::vector<khir::Value> initial_offset_values(child_operators.size(),
                                                 program_.ConstI32(-1));
  auto offset_array_type =
      program_.ArrayType(program_.I32Type(), child_operators.size());
  auto offset_array = program_.Global(
      false, true, offset_array_type,
      program_.ConstantArray(offset_array_type, initial_offset_values));

  // Setup table_ctr
  auto table_ctr_type = program_.ArrayType(program_.I32Type(), 1);
  auto table_ctr_ptr = program_.Global(
      false, true, table_ctr_type,
      program_.ConstantArray(table_ctr_type, {program_.ConstI32(0)}));

  // Setup last_table
  auto last_table_type = program_.ArrayType(program_.I32Type(), 1);
  auto last_table_ptr = program_.Global(
      false, true, last_table_type,
      program_.ConstantArray(last_table_type, {program_.ConstI32(0)}));

  // Setup # of result_tuples
  auto num_result_tuples_type = program_.ArrayType(program_.I32Type(), 1);
  auto num_result_tuples_ptr = program_.Global(
      false, true, num_result_tuples_type,
      program_.ConstantArray(num_result_tuples_type, {program_.ConstI32(0)}));

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
  auto flag_array = program_.Global(
      false, true, flag_array_type,
      program_.ConstantArray(
          flag_array_type,
          std::vector<khir::Value>(total_flags, program_.ConstI8(0))));

  // Setup table handlers
  auto handler_type = program_.FunctionType(
      program_.I32Type(), {program_.I32Type(), program_.I1Type()});
  auto handler_pointer_type = program_.PointerType(handler_type);
  auto handler_pointer_array_type =
      program_.ArrayType(handler_pointer_type, child_translators.size());
  auto handler_pointer_init = program_.ConstantArray(
      handler_pointer_array_type,
      std::vector<khir::Value>(child_translators.size(),
                               program_.NullPtr(handler_pointer_type)));
  auto handler_pointer_array = program_.Global(
      false, true, handler_pointer_array_type, handler_pointer_init);

  std::vector<proxy::TableFunction> table_functions;
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

  for (int table_idx = 0; table_idx < child_translators.size(); table_idx++) {
    table_functions.push_back(proxy::TableFunction(
        program_, [&](auto& initial_budget, auto& resume_progress) {
          auto handler_ptr =
              program_.GetElementPtr(handler_pointer_array_type,
                                     handler_pointer_array, {0, table_idx});
          auto handler = program_.LoadPtr(handler_ptr);

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

          auto& buffer = *materialized_buffers_[table_idx];
          auto cardinality = buffer.Size();

          // for each indexed predicate, if the buffer DNE, then return since
          // no result tuples with current column values.
          auto index_evaluted_predicates = IndexesAvailableForTable(
              table_idx, predicates_per_table, conditions);
          proxy::ColumnIndexBucketArray bucket_list(
              program_, index_evaluted_predicates.empty()
                            ? 1
                            : index_evaluted_predicates.size());

          for (int predicate_idx : index_evaluted_predicates) {
            const auto& predicate = conditions[predicate_idx].get();

            auto flag_ptr = program_.GetElementPtr(
                flag_array_type, flag_array,
                {0,
                 table_predicate_to_flag_idx.at({table_idx, predicate_idx})});
            proxy::Int8 flag_value(program_, program_.LoadI8(flag_ptr));
            proxy::If(program_, flag_value != 0, [&]() {
              auto eq =
                  dynamic_cast<const kush::plan::BinaryArithmeticExpression*>(
                      &predicate);
              auto left_column =
                  dynamic_cast<const kush::plan::ColumnRefExpression*>(
                      &eq->LeftChild());
              auto right_column =
                  dynamic_cast<const kush::plan::ColumnRefExpression*>(
                      &eq->RightChild());

              auto table_column = table_idx == left_column->GetChildIdx()
                                      ? left_column->GetColumnIdx()
                                      : right_column->GetColumnIdx();

              auto it = predicate_to_index_idx_.find({table_idx, table_column});
              assert(it != predicate_to_index_idx_.end());
              auto index_idx = it->second;

              auto other_side_value = expr_translator_.Compute(
                  table_idx == left_column->GetChildIdx() ? *right_column
                                                          : *left_column);

              proxy::If(
                  program_, other_side_value.IsNull(),
                  [&]() {
                    auto budget = initial_budget - 1;
                    proxy::If(
                        program_, budget == 0,
                        [&]() {
                          auto idx_ptr = program_.GetElementPtr(
                              idx_array_type, idx_array, {0, table_idx});
                          program_.StoreI32(idx_ptr, (cardinality - 1).Get());
                          program_.StoreI32(
                              program_.GetElementPtr(table_ctr_type,
                                                     table_ctr_ptr, {0, 0}),
                              program_.ConstI32(table_idx));
                          program_.Return(program_.ConstI32(-1));
                        },
                        [&]() { program_.Return(budget.Get()); });
                  },
                  [&]() {
                    auto bucket_from_index =
                        indexes_[index_idx]->GetBucket(other_side_value.Get());
                    bucket_list.PushBack(bucket_from_index);

                    proxy::If(
                        program_, bucket_from_index.DoesNotExist(), [&]() {
                          auto budget = initial_budget - 1;
                          proxy::If(
                              program_, budget == 0,
                              [&]() {
                                auto idx_ptr = program_.GetElementPtr(
                                    idx_array_type, idx_array, {0, table_idx});
                                program_.StoreI32(idx_ptr,
                                                  (cardinality - 1).Get());
                                program_.StoreI32(
                                    program_.GetElementPtr(
                                        table_ctr_type, table_ctr_ptr, {0, 0}),
                                    program_.ConstI32(table_idx));
                                program_.Return(program_.ConstI32(-1));
                              },
                              [&]() { program_.Return(budget.Get()); });
                        });
                  });
            });
          }

          auto use_index = proxy::Ternary(
              program_, bucket_list.Size() > 0,
              [&]() {
                auto bucket = bucket_list.Get(proxy::Int32(program_, 0));
                auto bucket_size = bucket.Size();

                proxy::Loop loop(
                    program_,
                    [&](auto& loop) {
                      auto progress_next_tuple = proxy::Ternary(
                          program_, resume_progress,
                          [&]() {
                            auto progress_ptr = program_.GetElementPtr(
                                progress_array_type, progress_arr,
                                {0, table_idx});
                            return proxy::Int32(program_,
                                                program_.LoadI32(progress_ptr));
                          },
                          [&]() { return proxy::Int32(program_, 0); });
                      auto offset_next_tuple =
                          proxy::Int32(program_,
                                       program_.LoadI32(program_.GetElementPtr(
                                           offset_array_type, offset_array,
                                           {0, table_idx}))) +
                          1;
                      auto initial_next_tuple = proxy::Ternary(
                          program_, offset_next_tuple > progress_next_tuple,
                          [&]() { return offset_next_tuple; },
                          [&]() { return progress_next_tuple; });
                      auto bucket_idx =
                          bucket.FastForwardToStart(initial_next_tuple);
                      loop.AddLoopVariable(bucket_idx);
                      loop.AddLoopVariable(initial_budget);

                      auto continue_resume_progress = proxy::Ternary(
                          program_, resume_progress,
                          [&]() {
                            return proxy::Ternary(
                                program_, bucket_idx < bucket_size,
                                [&]() {
                                  auto progress_ptr = program_.GetElementPtr(
                                      progress_array_type, progress_arr,
                                      {0, table_idx});
                                  auto progress_next_tuple = proxy::Int32(
                                      program_, program_.LoadI32(progress_ptr));

                                  auto bucket_next_tuple = bucket[bucket_idx];
                                  return bucket_next_tuple ==
                                         progress_next_tuple;
                                },
                                [&]() { return proxy::Bool(program_, false); });
                          },
                          [&]() { return proxy::Bool(program_, false); });
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

                      auto idx_ptr = program_.GetElementPtr(
                          idx_array_type, idx_array, {0, table_idx});
                      program_.StoreI32(idx_ptr, next_tuple.Get());

                      /*
                      proxy::Printer printer(program_);
                      std::string x;
                      for (int i = 0; i < curr; i++) {
                        x += " ";
                      }
                      auto str = proxy::String::Global(program_, x);
                      printer.Print(str);
                      printer.Print(next_tuple);
                      printer.PrintNewline();
                      */

                      auto current_table_values = buffer[next_tuple];

                      // Store each of this table's predicate column values
                      // into the global_predicate_struct
                      for (int k = 0; k < predicate_columns_.size(); k++) {
                        auto& col_ref = predicate_columns_[k].get();
                        if (col_ref.GetChildIdx() == table_idx) {
                          global_predicate_struct.Update(
                              k, current_table_values[col_ref.GetColumnIdx()]);

                          // Additionally, update this table's values to read
                          // from the unpacked tuple instead of the old loaded
                          // value from global_predicate_struct.
                          child_translators[table_idx]
                              .get()
                              .SchemaValues()
                              .SetValue(
                                  col_ref.GetColumnIdx(),
                                  std::move(current_table_values
                                                [col_ref.GetColumnIdx()]));
                        }
                      }

                      for (int predicate_idx :
                           predicates_per_table[table_idx]) {
                        auto flag_ptr = program_.GetElementPtr(
                            flag_array_type, flag_array,
                            {0, table_predicate_to_flag_idx.at(
                                    {table_idx, predicate_idx})});
                        proxy::Int8 flag_value(program_,
                                               program_.LoadI8(flag_ptr));
                        proxy::If(program_, flag_value != 0, [&]() {
                          auto cond = expr_translator_.Compute(
                              conditions[predicate_idx]);

                          proxy::If(
                              program_, cond.IsNull(),
                              [&]() {
                                // If budget, depleted return -1 and set
                                // table ctr
                                proxy::If(
                                    program_, budget == 0,
                                    [&]() {
                                      program_.StoreI32(
                                          program_.GetElementPtr(table_ctr_type,
                                                                 table_ctr_ptr,
                                                                 {0, 0}),
                                          program_.ConstI32(table_idx));
                                      program_.Return(program_.ConstI32(-1));
                                    },
                                    [&]() {
                                      loop.Continue(
                                          bucket_idx + 1, budget,
                                          proxy::Bool(program_, false));
                                    });
                              },
                              [&]() {
                                proxy::If(
                                    program_,
                                    !static_cast<proxy::Bool&>(cond.Get()),
                                    [&]() {
                                      // If budget, depleted return -1 and set
                                      // table ctr
                                      proxy::If(
                                          program_, budget == 0,
                                          [&]() {
                                            program_.StoreI32(
                                                program_.GetElementPtr(
                                                    table_ctr_type,
                                                    table_ctr_ptr, {0, 0}),
                                                program_.ConstI32(table_idx));
                                            program_.Return(
                                                program_.ConstI32(-1));
                                          },
                                          [&]() {
                                            loop.Continue(
                                                bucket_idx + 1, budget,
                                                proxy::Bool(program_, false));
                                          });
                                    });
                              });
                        });
                      }

                      proxy::If(program_, budget == 0, [&]() {
                        program_.StoreI32(
                            program_.GetElementPtr(table_ctr_type,
                                                   table_ctr_ptr, {0, 0}),
                            program_.ConstI32(table_idx));
                        program_.Return(program_.ConstI32(-2));
                      });

                      // Valid tuple
                      auto next_budget = proxy::Int32(
                          program_,
                          program_.Call(handler,
                                        {budget.Get(), resume_progress.Get()}));
                      proxy::If(program_, next_budget < 0,
                                [&]() { program_.Return(next_budget.Get()); });

                      return loop.Continue(bucket_idx + 1, next_budget,
                                           proxy::Bool(program_, false));
                    });

                return loop.template GetLoopVariable<proxy::Int32>(1);
              },
              [&]() {
                // Loop over tuples in buffer
                proxy::Loop loop(
                    program_,
                    [&](auto& loop) {
                      auto progress_next_tuple = proxy::Ternary(
                          program_, resume_progress,
                          [&]() {
                            auto progress_ptr = program_.GetElementPtr(
                                progress_array_type, progress_arr,
                                {0, table_idx});
                            return proxy::Int32(program_,
                                                program_.LoadI32(progress_ptr));
                          },
                          [&]() { return proxy::Int32(program_, 0); });
                      auto offset_next_tuple =
                          proxy::Int32(program_,
                                       program_.LoadI32(program_.GetElementPtr(
                                           offset_array_type, offset_array,
                                           {0, table_idx}))) +
                          1;
                      auto initial_next_tuple = proxy::Ternary(
                          program_, offset_next_tuple > progress_next_tuple,
                          [&]() { return offset_next_tuple; },
                          [&]() { return progress_next_tuple; });

                      loop.AddLoopVariable(initial_next_tuple);
                      loop.AddLoopVariable(initial_budget);

                      auto continue_resume_progress = proxy::Ternary(
                          program_, resume_progress,
                          [&]() {
                            auto progress_ptr = program_.GetElementPtr(
                                progress_array_type, progress_arr,
                                {0, table_idx});
                            auto progress_next_tuple = proxy::Int32(
                                program_, program_.LoadI32(progress_ptr));
                            return initial_next_tuple == progress_next_tuple;
                          },
                          [&]() { return proxy::Bool(program_, false); });
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

                      auto idx_ptr = program_.GetElementPtr(
                          idx_array_type, idx_array, {0, table_idx});
                      program_.StoreI32(idx_ptr, next_tuple.Get());

                      /*
                      proxy::Printer printer(program_);
                      std::string x;
                      for (int i = 0; i < curr; i++) {
                        x += " ";
                      }
                      auto str = proxy::String::Global(program_, x);
                      printer.Print(str);
                      printer.Print(next_tuple);
                      printer.PrintNewline();
                      */
                      auto current_table_values = buffer[next_tuple];

                      // Store each of this table's predicate column values
                      // into the global_predicate_struct
                      for (int k = 0; k < predicate_columns_.size(); k++) {
                        auto& col_ref = predicate_columns_[k].get();
                        if (col_ref.GetChildIdx() == table_idx) {
                          global_predicate_struct.Update(
                              k, current_table_values[col_ref.GetColumnIdx()]);

                          // Additionally, update this table's values to read
                          // from the unpacked tuple instead of the old loaded
                          // value from global_predicate_struct.
                          child_translators[table_idx]
                              .get()
                              .SchemaValues()
                              .SetValue(
                                  col_ref.GetColumnIdx(),
                                  std::move(current_table_values
                                                [col_ref.GetColumnIdx()]));
                        }
                      }

                      for (int predicate_idx :
                           predicates_per_table[table_idx]) {
                        auto flag_ptr = program_.GetElementPtr(
                            flag_array_type, flag_array,
                            {0, table_predicate_to_flag_idx.at(
                                    {table_idx, predicate_idx})});
                        proxy::Int8 flag_value(program_,
                                               program_.LoadI8(flag_ptr));
                        proxy::If(program_, flag_value != 0, [&]() {
                          auto cond = expr_translator_.Compute(
                              conditions[predicate_idx]);

                          proxy::If(
                              program_, cond.IsNull(),
                              [&]() {
                                // If budget, depleted return -1 and set
                                // table ctr
                                proxy::If(
                                    program_, budget == 0,
                                    [&]() {
                                      program_.StoreI32(
                                          program_.GetElementPtr(table_ctr_type,
                                                                 table_ctr_ptr,
                                                                 {0, 0}),
                                          program_.ConstI32(table_idx));
                                      program_.Return(program_.ConstI32(-1));
                                    },
                                    [&]() {
                                      loop.Continue(
                                          next_tuple + 1, budget,
                                          proxy::Bool(program_, false));
                                    });
                              },
                              [&]() {
                                proxy::If(
                                    program_,
                                    !static_cast<proxy::Bool&>(
                                        cond.Get() /*cond*/),
                                    [&]() {
                                      // If budget, depleted return -1 and set
                                      // table ctr
                                      proxy::If(
                                          program_, budget == 0,
                                          [&]() {
                                            program_.StoreI32(
                                                program_.GetElementPtr(
                                                    table_ctr_type,
                                                    table_ctr_ptr, {0, 0}),
                                                program_.ConstI32(table_idx));
                                            program_.Return(
                                                program_.ConstI32(-1));
                                          },
                                          [&]() {
                                            loop.Continue(
                                                next_tuple + 1, budget,
                                                proxy::Bool(program_, false));
                                          });
                                    });
                              });
                        });
                      }

                      proxy::If(program_, budget == 0, [&]() {
                        program_.StoreI32(
                            program_.GetElementPtr(table_ctr_type,
                                                   table_ctr_ptr, {0, 0}),
                            program_.ConstI32(table_idx));
                        program_.Return(program_.ConstI32(-2));
                      });

                      // Valid tuple
                      auto next_budget = proxy::Int32(
                          program_,
                          program_.Call(handler,
                                        {budget.Get(), resume_progress.Get()}));
                      proxy::If(program_, next_budget < 0,
                                [&]() { program_.Return(next_budget.Get()); });

                      return loop.Continue(next_tuple + 1, next_budget,
                                           proxy::Bool(program_, false));
                    });

                return loop.template GetLoopVariable<proxy::Int32>(1);
              });

          return use_index;
        }));
  }

  // Setup global hash table that contains tuple idx
  proxy::TupleIdxTable tuple_idx_table(program_, true);

  // Setup function for each valid tuple
  proxy::TableFunction valid_tuple_handler(
      program_, [&](const auto& budget, const auto& resume_progress) {
        // Insert tuple idx into hash table
        auto tuple_idx_arr =
            program_.GetElementPtr(idx_array_type, idx_array, {0, 0});

        proxy::Int32 num_tables(program_, child_translators.size());
        tuple_idx_table.Insert(tuple_idx_arr, num_tables);

        auto result_ptr = program_.GetElementPtr(num_result_tuples_type,
                                                 num_result_tuples_ptr, {0, 0});
        proxy::Int32 num_result_tuples(program_, program_.LoadI32(result_ptr));
        program_.StoreI32(result_ptr, (num_result_tuples + 1).Get());

        return budget;
      });

  // 3. Execute join
  // Initialize handler array with the corresponding functions for each table
  for (int i = 0; i < child_operators.size(); i++) {
    auto handler_ptr = program_.GetElementPtr(handler_pointer_array_type,
                                              handler_pointer_array, {0, i});
    program_.StorePtr(handler_ptr,
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
  auto table_predicate_to_flag_idx_arr =
      program_.Global(true, true, table_predicate_to_flag_idx_arr_type,
                      table_predicate_to_flag_idx_arr_init);

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

  // Write out cardinalities to idx_array
  for (int i = 0; i < child_operators.size(); i++) {
    program_.StoreI32(program_.GetElementPtr(idx_array_type, idx_array, {0, i}),
                      materialized_buffers_[i]->Size().Get());
  }

  // Execute build side of skinner join
  proxy::SkinnerJoinExecutor executor(program_);

  executor.ExecutePermutableJoin({
      program_.ConstI32(child_operators.size()),
      program_.ConstI32(conditions.size()),
      program_.GetElementPtr(handler_pointer_array_type, handler_pointer_array,
                             {0, 0}),
      program_.GetFunctionPointer(valid_tuple_handler.Get()),
      program_.ConstI32(table_predicate_to_flag_idx_values.size()),
      program_.GetElementPtr(table_predicate_to_flag_idx_arr_type,
                             table_predicate_to_flag_idx_arr, {0, 0}),
      program_.GetElementPtr(tables_per_predicate_arr_type,
                             tables_per_predicate_arr, {0, 0}),
      program_.GetElementPtr(flag_array_type, flag_array, {0, 0}),
      program_.GetElementPtr(progress_array_type, progress_arr, {0, 0}),
      program_.GetElementPtr(table_ctr_type, table_ctr_ptr, {0, 0}),
      program_.GetElementPtr(idx_array_type, idx_array, {0, 0}),
      program_.GetElementPtr(last_table_type, last_table_ptr, {0, 0}),
      program_.GetElementPtr(num_result_tuples_type, num_result_tuples_ptr,
                             {0, 0}),
      program_.GetElementPtr(offset_array_type, offset_array, {0, 0}),
  });
  program_.Return();
  auto join_pipeline_obj = pipeline_builder_.FinishPipeline();

  program_.SetCurrentBlock(output_pipeline_func);
  auto& output_pipeline = pipeline_builder_.GetCurrentPipeline();
  output_pipeline.AddPredecessor(std::move(join_pipeline_obj));

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
  predicate_struct.reset();
}

void PermutableSkinnerJoinTranslator::Consume(OperatorTranslator& src) {
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