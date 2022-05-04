#include "compile/translators/permutable_skinner_join_translator.h"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"

#include "compile/proxy/control_flow/if.h"
#include "compile/proxy/control_flow/loop.h"
#include "compile/proxy/disk_column_index.h"
#include "compile/proxy/function.h"
#include "compile/proxy/memory_column_index.h"
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

class TableFunction {
 public:
  TableFunction(khir::ProgramBuilder& program,
                std::function<proxy::Int32(proxy::Int32&, proxy::Bool&)> body) {
    auto current_block = program.CurrentBlock();

    func_ = program.CreatePublicFunction(
        program.I32Type(), {program.I32Type(), program.I1Type()},
        "permute_table_" + std::to_string(table_++));

    auto args = program.GetFunctionArguments(func_.value());
    proxy::Int32 budget(program, args[0]);
    proxy::Bool resume_progress(program, args[1]);

    auto x = body(budget, resume_progress);
    program.Return(x.Get());

    program.SetCurrentBlock(current_block);
  }

  khir::FunctionRef Get() { return func_.value(); }

 private:
  std::optional<khir::FunctionRef> func_;
  static int table_;
};

int TableFunction::table_ = 0;

std::unique_ptr<proxy::ColumnIndex> GenerateInMemoryIndex(
    khir::ProgramBuilder& program, const catalog::Type& type) {
  return std::make_unique<proxy::MemoryColumnIndex>(program, type);
}

bool IsEqualityPredicate(std::reference_wrapper<const plan::Expression> expr) {
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

PermutableSkinnerJoinTranslator::PermutableSkinnerJoinTranslator(
    const plan::SkinnerJoinOperator& join, khir::ProgramBuilder& program,
    execution::PipelineBuilder& pipeline_builder,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(join, std::move(children)),
      join_(join),
      program_(program),
      pipeline_builder_(pipeline_builder),
      expr_translator_(program_, *this) {}

void PermutableSkinnerJoinTranslator::Produce() {
  auto output_pipeline_func = program_.CurrentBlock();

  auto child_translators = this->Children();
  auto child_operators = this->join_.Children();
  auto conditions = join_.Conditions();

  int index_idx = 0;
  for (int i = 0; i < conditions.size(); i++) {
    const auto& condition = conditions[i].get();

    PredicateColumnCollector collector;
    condition.Accept(collector);
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
          indexes_[index_idx] = GenerateInMemoryIndex(program_, type);
          indexes_[index_idx]->Init();
          disk_materialized_buffer->Scan(
              col_idx, [&](auto tuple_idx, auto value) {
                // only index not null values
                proxy::If(program_, NOT, value.IsNull(), [&]() {
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
      for (const auto& [key, value] : column_to_index_idx_) {
        auto [child_idx, col_idx] = key;
        auto index_idx = value;
        if (i != child_idx) {
          continue;
        }

        auto type = child_operator.Schema().Columns()[col_idx].Expr().Type();
        indexes_[index_idx] = GenerateInMemoryIndex(program_, type);
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
  // Setup struct of predicate columns
  // - 1 for the equality columns
  // - add remaining for each general column
  auto predicate_struct = std::make_unique<proxy::StructBuilder>(program_);
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
      program_, *predicate_struct,
      program_.Global(
          predicate_struct->Type(),
          program_.ConstantStruct(predicate_struct->Type(),
                                  predicate_struct->DefaultValues())));

  // Setup idx array.
  std::vector<khir::Value> initial_idx_values(child_operators.size(),
                                              program_.ConstI32(0));
  auto idx_array_type =
      program_.ArrayType(program_.I32Type(), child_operators.size());
  auto idx_array = program_.Global(
      idx_array_type,
      program_.ConstantArray(idx_array_type, initial_idx_values));

  // Setup progress array
  std::vector<khir::Value> initial_progress_values(child_operators.size(),
                                                   program_.ConstI32(-1));
  auto progress_array_type =
      program_.ArrayType(program_.I32Type(), child_operators.size());
  auto progress_arr = program_.Global(
      progress_array_type,
      program_.ConstantArray(progress_array_type, initial_progress_values));

  // Setup offset array
  std::vector<khir::Value> initial_offset_values(child_operators.size(),
                                                 program_.ConstI32(-1));
  auto offset_array_type =
      program_.ArrayType(program_.I32Type(), child_operators.size());
  auto offset_array = program_.Global(
      offset_array_type,
      program_.ConstantArray(offset_array_type, initial_offset_values));

  // Setup table_ctr
  auto table_ctr_type = program_.ArrayType(program_.I32Type(), 1);
  auto table_ctr_ptr = program_.Global(
      table_ctr_type,
      program_.ConstantArray(table_ctr_type, {program_.ConstI32(0)}));

  // Setup # of result_tuples
  auto num_result_tuples_type = program_.ArrayType(program_.I32Type(), 1);
  auto num_result_tuples_ptr = program_.Global(
      num_result_tuples_type,
      program_.ConstantArray(num_result_tuples_type, {program_.ConstI32(0)}));

  // Setup flag array for each table.
  // allocate a flag for each:
  // table in a predicate
  int total_flags = 0;
  for (int pred = 0; pred < conditions.size(); pred++) {
    PredicateColumnCollector collector;
    conditions[pred].get().Accept(collector);
    auto cond = collector.PredicateColumns();

    absl::flat_hash_set<int> tables;
    for (const auto& col_ref : cond) {
      tables.insert(col_ref.get().GetChildIdx());
    }

    for (auto t : tables) {
      if (!join_.PrefixOrder().empty() && join_.PrefixOrder()[0] == t) {
        continue;
      }

      pred_table_to_flag_[{pred, t}] = total_flags++;
    }
  }

  auto flag_type = program_.I8Type();
  auto flag_array_type = program_.ArrayType(flag_type, total_flags);
  auto flag_array = program_.Global(
      flag_array_type,
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
  auto handler_pointer_array =
      program_.Global(handler_pointer_array_type, handler_pointer_init);

  std::vector<TableFunction> table_functions;
  // initially fill the child_translators schema values with garbage
  // this will get overwritten when we actually load tuples/update predicate
  // struct
  for (int i = 0; i < child_translators.size(); i++) {
    auto& child_translator = child_translators[i].get();
    auto& child_operator = child_operators[i].get();

    const auto& schema = child_operator.Schema().Columns();

    child_translator.SchemaValues().ResetValues();
    for (int i = 0; i < schema.size(); i++) {
      const auto& type = schema[i].Expr().Type();
      switch (type.type_id) {
        case catalog::TypeId::SMALLINT:
          child_translator.SchemaValues().AddVariable(proxy::SQLValue(
              proxy::Int16(program_, 0), proxy::Bool(program_, false)));
          break;
        case catalog::TypeId::INT:
          child_translator.SchemaValues().AddVariable(proxy::SQLValue(
              proxy::Int32(program_, 0), proxy::Bool(program_, false)));
          break;
        case catalog::TypeId::DATE:
          child_translator.SchemaValues().AddVariable(proxy::SQLValue(
              proxy::Date(program_, runtime::Date::DateBuilder(2000, 1, 1)),
              proxy::Bool(program_, false)));
          break;
        case catalog::TypeId::BIGINT:
          child_translator.SchemaValues().AddVariable(proxy::SQLValue(
              proxy::Int64(program_, 0), proxy::Bool(program_, false)));
          break;
        case catalog::TypeId::BOOLEAN:
          child_translator.SchemaValues().AddVariable(proxy::SQLValue(
              proxy::Bool(program_, false), proxy::Bool(program_, false)));
          break;
        case catalog::TypeId::REAL:
          child_translator.SchemaValues().AddVariable(proxy::SQLValue(
              proxy::Float64(program_, 0), proxy::Bool(program_, false)));
          break;
        case catalog::TypeId::TEXT:
          child_translator.SchemaValues().AddVariable(
              proxy::SQLValue(proxy::String::Global(program_, ""),
                              proxy::Bool(program_, false)));
          break;
        case catalog::TypeId::ENUM:
          child_translator.SchemaValues().AddVariable(
              proxy::SQLValue(proxy::Enum(program_, type.enum_id, -1),
                              proxy::Bool(program_, false)));
          break;
      }
    }
  }

  int bucket_list_max_size = predicate_columns_.size();
  for (int table_idx = 0; table_idx < child_translators.size(); table_idx++) {
    table_functions.push_back(TableFunction(program_, [&](auto& initial_budget,
                                                          auto&
                                                              resume_progress) {
      auto handler_ptr = program_.StaticGEP(
          handler_pointer_array_type, handler_pointer_array, {0, table_idx});
      auto handler = program_.LoadPtr(handler_ptr);

      {
        // Unpack the predicate struct.
        auto column_values = global_predicate_struct.Unpack();

        for (const auto& [colref, field] : colref_to_predicate_struct_field_) {
          auto [child_idx, col_idx] = colref;

          // Set the ColumnIdx value of the ChildIdx operator to be the
          // unpacked value.
          auto& child_translator = child_translators[child_idx].get();
          child_translator.SchemaValues().SetValue(col_idx,
                                                   column_values[field]);
        }
      }

      auto& buffer = *materialized_buffers_[table_idx];
      auto cardinality = buffer.Size();

      // for each equality predicate, if the buffer DNE, then return since
      // no result tuples with current column values.
      proxy::ColumnIndexBucketArray bucket_list(program_, bucket_list_max_size);

      for (auto [pred_table, flag] : pred_table_to_flag_) {
        if (pred_table.second != table_idx) {
          continue;
        }

        auto cond = conditions[pred_table.first];
        if (!IsEqualityPredicate(cond)) {
          continue;
        }
        const auto& eq =
            dynamic_cast<const plan::BinaryArithmeticExpression&>(cond.get());
        const auto& left =
            dynamic_cast<const plan::ColumnRefExpression&>(eq.LeftChild());
        const auto& right =
            dynamic_cast<const plan::ColumnRefExpression&>(eq.RightChild());

        auto flag_ptr =
            program_.StaticGEP(flag_array_type, flag_array, {0, flag});
        proxy::Int8 flag_value(program_, program_.LoadI8(flag_ptr));

        proxy::If(program_, flag_value != 0, [&]() {
          auto other_side_value = left.GetChildIdx() == table_idx
                                      ? expr_translator_.Compute(right)
                                      : expr_translator_.Compute(left);
          auto table_column = left.GetChildIdx() == table_idx
                                  ? left.GetColumnIdx()
                                  : right.GetColumnIdx();

          proxy::If(
              program_, other_side_value.IsNull(),
              [&]() {
                auto budget = initial_budget - 1;
                proxy::If(
                    program_, budget == 0,
                    [&]() {
                      auto idx_ptr = program_.StaticGEP(
                          idx_array_type, idx_array, {0, table_idx});
                      program_.StoreI32(idx_ptr, (cardinality - 1).Get());
                      program_.StoreI32(
                          program_.StaticGEP(table_ctr_type, table_ctr_ptr,
                                             {0, 0}),
                          program_.ConstI32(table_idx));
                      program_.Return(program_.ConstI32(-1));
                    },
                    [&]() { program_.Return(budget.Get()); });
              },
              [&]() {
                auto it = column_to_index_idx_.find({table_idx, table_column});
                if (it == column_to_index_idx_.end()) {
                  throw std::runtime_error("Expected index.");
                }
                auto index_idx = it->second;

                auto bucket_from_index =
                    indexes_[index_idx]->GetBucket(other_side_value.Get());
                bucket_list.PushBack(bucket_from_index);
                proxy::If(program_, bucket_from_index.DoesNotExist(), [&]() {
                  auto budget = initial_budget - 1;
                  proxy::If(
                      program_, budget == 0,
                      [&]() {
                        auto idx_ptr = program_.StaticGEP(
                            idx_array_type, idx_array, {0, table_idx});
                        program_.StoreI32(idx_ptr, (cardinality - 1).Get());
                        program_.StoreI32(
                            program_.StaticGEP(table_ctr_type, table_ctr_ptr,
                                               {0, 0}),
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
            auto progress_next_tuple = proxy::Ternary(
                program_, resume_progress,
                [&]() {
                  auto progress_ptr = program_.StaticGEP(
                      progress_array_type, progress_arr, {0, table_idx});
                  return proxy::Int32(program_, program_.LoadI32(progress_ptr));
                },
                [&]() { return proxy::Int32(program_, 0); });
            auto offset_next_tuple =
                proxy::Int32(program_, program_.LoadI32(program_.StaticGEP(
                                           offset_array_type, offset_array,
                                           {0, table_idx}))) +
                1;
            auto initial_next_tuple = proxy::Ternary(
                program_, offset_next_tuple > progress_next_tuple,
                [&]() { return offset_next_tuple; },
                [&]() { return progress_next_tuple; });

            bucket_list.InitSortedIntersection(initial_next_tuple);

            int result_max_size = 64;
            auto result_array_type =
                program_.ArrayType(program_.I32Type(), result_max_size);
            std::vector<khir::Value> initial_result_values(
                result_max_size, program_.ConstI32(0));
            auto result_array =
                program_.Global(result_array_type,
                                program_.ConstantArray(result_array_type,
                                                       initial_result_values));
            auto result =
                program_.StaticGEP(result_array_type, result_array, {0, 0});

            auto result_initial_size =
                bucket_list.PopulateSortedIntersectionResult(result,
                                                             result_max_size);

            proxy::Loop loop(
                program_,
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
                      program_,
                      [&](auto& loop) {
                        loop.AddLoopVariable(proxy::Int32(program_, 0));
                        loop.AddLoopVariable(budget);

                        auto continue_resume_progress = proxy::Ternary(
                            program_, resume_progress,
                            [&]() {
                              auto bucket_next_tuple_ptr = program_.StaticGEP(
                                  program_.I32Type(), result, {0});
                              auto initial_next_tuple = proxy::Int32(
                                  program_,
                                  program_.LoadI32(bucket_next_tuple_ptr));
                              return initial_next_tuple == progress_next_tuple;
                            },
                            [&]() { return proxy::Bool(program_, false); });
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
                            program_, result, bucket_idx);

                        auto idx_ptr = program_.StaticGEP(
                            idx_array_type, idx_array, {0, table_idx});
                        program_.StoreI32(idx_ptr, next_tuple.Get());

                        /*
                        proxy::Printer printer(program_, true);
                        printer.Print(proxy::Int32(program_, table_idx));
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

                        for (auto [pred_table, flag] : pred_table_to_flag_) {
                          auto pred = pred_table.first;
                          auto table = pred_table.second;
                          if (table != table_idx) {
                            continue;
                          }

                          auto flag_ptr = program_.StaticGEP(
                              flag_array_type, flag_array, {0, flag});
                          proxy::Int8 flag_value(program_,
                                                 program_.LoadI8(flag_ptr));
                          proxy::If(program_, flag_value != 0, [&]() {
                            auto cond = expr_translator_.Compute(
                                conditions[pred].get());

                            proxy::If(
                                program_, cond.IsNull(),
                                [&]() {
                                  // If budget, depleted return -1 and set
                                  // table ctr
                                  proxy::If(
                                      program_, budget == 0,
                                      [&]() {
                                        program_.StoreI32(
                                            program_.StaticGEP(table_ctr_type,
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
                                      program_, NOT,
                                      static_cast<proxy::Bool&>(cond.Get()),
                                      [&]() {
                                        // If budget, depleted return -1 and
                                        // set table ctr
                                        proxy::If(
                                            program_, budget == 0,
                                            [&]() {
                                              program_.StoreI32(
                                                  program_.StaticGEP(
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
                              program_.StaticGEP(table_ctr_type, table_ctr_ptr,
                                                 {0, 0}),
                              program_.ConstI32(table_idx));
                          program_.Return(program_.ConstI32(-2));
                        });

                        for (auto [col_idx, field] : cols) {
                          auto value = child_translators[table_idx]
                                           .get()
                                           .SchemaValues()
                                           .Value(col_idx);
                          global_predicate_struct.Update(field, value);
                        }

                        // Valid tuple
                        auto next_budget = proxy::Int32(
                            program_,
                            program_.Call(handler, {budget.Get(),
                                                    resume_progress.Get()}));
                        proxy::If(program_, next_budget < 0, [&]() {
                          program_.Return(next_budget.Get());
                        });

                        return loop.Continue(bucket_idx + 1, next_budget,
                                             proxy::Bool(program_, false));
                      });

                  auto next_budget =
                      result_loop.template GetLoopVariable<proxy::Int32>(1);
                  auto next_result_size =
                      bucket_list.PopulateSortedIntersectionResult(
                          result, result_max_size);
                  return loop.Continue(next_budget, next_result_size);
                });

            return loop.template GetLoopVariable<proxy::Int32>(0);
          },
          [&]() {
            // Loop over tuples in buffer
            proxy::Loop loop(
                program_,
                [&](auto& loop) {
                  auto progress_next_tuple = proxy::Ternary(
                      program_, resume_progress,
                      [&]() {
                        auto progress_ptr = program_.StaticGEP(
                            progress_array_type, progress_arr, {0, table_idx});
                        return proxy::Int32(program_,
                                            program_.LoadI32(progress_ptr));
                      },
                      [&]() { return proxy::Int32(program_, 0); });
                  auto offset_next_tuple =
                      proxy::Int32(program_,
                                   program_.LoadI32(program_.StaticGEP(
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
                        auto progress_ptr = program_.StaticGEP(
                            progress_array_type, progress_arr, {0, table_idx});
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

                  auto idx_ptr = program_.StaticGEP(idx_array_type, idx_array,
                                                    {0, table_idx});
                  program_.StoreI32(idx_ptr, next_tuple.Get());

                  /*
                  proxy::Printer printer(program_, true);
                  printer.Print(proxy::Int32(program_, table_idx));
                  printer.Print(next_tuple);
                  printer.PrintNewline();
                  */

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

                  for (auto [pred_table, flag] : pred_table_to_flag_) {
                    auto pred = pred_table.first;
                    auto table = pred_table.second;
                    if (table != table_idx) {
                      continue;
                    }

                    auto flag_ptr = program_.StaticGEP(flag_array_type,
                                                       flag_array, {0, flag});
                    proxy::Int8 flag_value(program_, program_.LoadI8(flag_ptr));
                    proxy::If(program_, flag_value != 0, [&]() {
                      auto cond =
                          expr_translator_.Compute(conditions[pred].get());

                      proxy::If(
                          program_, cond.IsNull(),
                          [&]() {
                            // If budget, depleted return -1 and set
                            // table ctr
                            proxy::If(
                                program_, budget == 0,
                                [&]() {
                                  program_.StoreI32(
                                      program_.StaticGEP(table_ctr_type,
                                                         table_ctr_ptr, {0, 0}),
                                      program_.ConstI32(table_idx));
                                  program_.Return(program_.ConstI32(-1));
                                },
                                [&]() {
                                  loop.Continue(next_tuple + 1, budget,
                                                proxy::Bool(program_, false));
                                });
                          },
                          [&]() {
                            proxy::If(
                                program_, NOT,
                                static_cast<proxy::Bool&>(cond.Get()), [&]() {
                                  // If budget, depleted return -1 and set
                                  // table ctr
                                  proxy::If(
                                      program_, budget == 0,
                                      [&]() {
                                        program_.StoreI32(
                                            program_.StaticGEP(table_ctr_type,
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
                                });
                          });
                    });
                  }

                  proxy::If(program_, budget == 0, [&]() {
                    program_.StoreI32(program_.StaticGEP(table_ctr_type,
                                                         table_ctr_ptr, {0, 0}),
                                      program_.ConstI32(table_idx));
                    program_.Return(program_.ConstI32(-2));
                  });

                  for (auto [col_idx, field] : cols) {
                    auto value =
                        child_translators[table_idx].get().SchemaValues().Value(
                            col_idx);
                    global_predicate_struct.Update(field, value);
                  }

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
  proxy::TupleIdxTable tuple_idx_table(program_);

  // Setup function for each valid tuple
  TableFunction valid_tuple_handler(program_, [&](const auto& budget,
                                                  const auto& resume_progress) {
    // Insert tuple idx into hash table
    auto tuple_idx_arr = program_.StaticGEP(idx_array_type, idx_array, {0, 0});

    proxy::Int32 num_tables(program_, child_translators.size());
    tuple_idx_table.Insert(tuple_idx_arr, num_tables);

    auto result_ptr = program_.StaticGEP(num_result_tuples_type,
                                         num_result_tuples_ptr, {0, 0});
    proxy::Int32 num_result_tuples(program_, program_.LoadI32(result_ptr));
    program_.StoreI32(result_ptr, (num_result_tuples + 1).Get());

    return budget;
  });

  // 3. Execute join
  // Initialize handler array with the corresponding functions for each table
  for (int i = 0; i < child_operators.size(); i++) {
    auto handler_ptr = program_.StaticGEP(handler_pointer_array_type,
                                          handler_pointer_array, {0, i});
    program_.StorePtr(handler_ptr,
                      {program_.GetFunctionPointer(table_functions[i].Get())});
  }

  // Write out cardinalities to idx_array
  for (int i = 0; i < child_operators.size(); i++) {
    program_.StoreI32(program_.StaticGEP(idx_array_type, idx_array, {0, i}),
                      materialized_buffers_[i]->Size().Get());
  }

  // Generate the table connections.
  for (int pred_idx = 0; pred_idx < conditions.size(); pred_idx++) {
    PredicateColumnCollector collector;
    conditions[pred_idx].get().Accept(collector);
    auto cond = collector.PredicateColumns();

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

  // Execute build side of skinner join
  proxy::SkinnerJoinExecutor::ExecutePermutableJoin(
      program_, child_operators.size(), conditions.size(), &pred_table_to_flag_,
      &table_connections_, &join_.PrefixOrder(),
      program_.StaticGEP(handler_pointer_array_type, handler_pointer_array,
                         {0, 0}),
      program_.GetFunctionPointer(valid_tuple_handler.Get()), total_flags,
      program_.StaticGEP(flag_array_type, flag_array, {0, 0}),
      program_.StaticGEP(progress_array_type, progress_arr, {0, 0}),
      program_.StaticGEP(table_ctr_type, table_ctr_ptr, {0, 0}),
      program_.StaticGEP(idx_array_type, idx_array, {0, 0}),
      program_.StaticGEP(num_result_tuples_type, num_result_tuples_ptr, {0, 0}),
      program_.StaticGEP(offset_array_type, offset_array, {0, 0}));
  program_.Return();
  auto join_pipeline_obj = pipeline_builder_.FinishPipeline();

  program_.SetCurrentBlock(output_pipeline_func);
  auto& output_pipeline = pipeline_builder_.GetCurrentPipeline();
  output_pipeline.AddPredecessor(std::move(join_pipeline_obj));

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
      const auto& type = schema[i].Expr().Type();
      switch (type.type_id) {
        case catalog::TypeId::SMALLINT:
          child_translator.SchemaValues().AddVariable(proxy::SQLValue(
              proxy::Int16(program_, 0), proxy::Bool(program_, false)));
          break;
        case catalog::TypeId::INT:
          child_translator.SchemaValues().AddVariable(proxy::SQLValue(
              proxy::Int32(program_, 0), proxy::Bool(program_, false)));
          break;
        case catalog::TypeId::DATE:
          child_translator.SchemaValues().AddVariable(proxy::SQLValue(
              proxy::Date(program_, runtime::Date::DateBuilder(2000, 1, 1)),
              proxy::Bool(program_, false)));
          break;
        case catalog::TypeId::BIGINT:
          child_translator.SchemaValues().AddVariable(proxy::SQLValue(
              proxy::Int64(program_, 0), proxy::Bool(program_, false)));
          break;
        case catalog::TypeId::BOOLEAN:
          child_translator.SchemaValues().AddVariable(proxy::SQLValue(
              proxy::Bool(program_, false), proxy::Bool(program_, false)));
          break;
        case catalog::TypeId::REAL:
          child_translator.SchemaValues().AddVariable(proxy::SQLValue(
              proxy::Float64(program_, 0), proxy::Bool(program_, false)));
          break;
        case catalog::TypeId::TEXT:
          child_translator.SchemaValues().AddVariable(
              proxy::SQLValue(proxy::String::Global(program_, ""),
                              proxy::Bool(program_, false)));
          break;
        case catalog::TypeId::ENUM:
          child_translator.SchemaValues().AddVariable(
              proxy::SQLValue(proxy::Enum(program_, type.enum_id, -1),
                              proxy::Bool(program_, false)));
          break;
      }
    }
  }

  // Loop over tuple idx table and then output tuples from each table.
  tuple_idx_table.ForEach([&](const auto& tuple_idx_arr) {
    int current_buffer = 0;
    for (int i = 0; i < child_translators.size(); i++) {
      auto& child_translator = child_translators[i].get();

      auto tuple_idx_ptr =
          program_.StaticGEP(program_.I32Type(), tuple_idx_arr, {i});
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
  predicate_struct.reset();
}

void PermutableSkinnerJoinTranslator::Consume(OperatorTranslator& src) {
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
      proxy::If(program_, NOT, value.IsNull(), [&]() {
        dynamic_cast<proxy::ColumnIndexBuilder*>(indexes_[index_idx].get())
            ->Insert(value.Get(), tuple_idx);
      });
    }
  }

  // insert tuple into buffer
  buffer_->PushBack().Pack(values);
}

}  // namespace kush::compile