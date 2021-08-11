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
/*
void RecompilingSkinnerJoinTranslator::GenerateChildLoops(
    khir::ProgramBuilder& program, const std::vector<int>& order, int curr,
    absl::flat_hash_set<int> evaluated_predicates,
    std::vector<absl::btree_set<int>>& predicates_per_table,
    absl::flat_hash_set<int> available_tables) {
  int table_idx = order[curr];
  auto conditions = join_.Conditions();
  auto& buffer = buffers_[table_idx];
  auto cardinality = buffer.Size();
  auto child_translators = this->Children();
  proxy::Loop loop(
      program,
      [&](auto& loop) { loop.AddLoopVariable(proxy::Int32(program, 0)); },
      [&](auto& loop) {
        auto last_tuple = loop.template GetLoopVariable<proxy::Int32>(0);
        return last_tuple < (cardinality - 1);
      },
      [&](auto& loop) {
        auto last_tuple = loop.template GetLoopVariable<proxy::Int32>(0);

        auto next_tuple = (last_tuple + 1).ToPointer();

        // Get next_tuple from active equality predicates
        for (int predicate_idx : predicates_per_table[table_idx]) {
          if (evaluated_predicates.contains(predicate_idx)) {
            continue;
          }
          evaluated_predicates.insert(predicate_idx);

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

                    auto other_side_value = expr_translator_.Compute(
                        table_idx == left_column->GetChildIdx() ? *right_column
                                                                : *left_column);
                    auto next_greater_in_index = indexes_[idx]->GetNextGreater(
                        *other_side_value, last_tuple, cardinality);

                    auto max_check = proxy::If(
                        program_, next_greater_in_index > (*next_tuple),
                        [&]() -> std::vector<khir::Value> {
                          return {next_greater_in_index.Get()};
                        },
                        [&]() -> std::vector<khir::Value> {
                          return {next_tuple->Get()};
                        })[0];
                    next_tuple = proxy::Int32(program_, max_check).ToPointer();
                  }
                }
              }
            }
          }
        }

        proxy::If(
            program_, *next_tuple == cardinality,
            [&]() -> std::vector<khir::Value> {
              loop.Break();
              return {};
            },
            []() -> std::vector<khir::Value> { return {}; });

        auto tuple = buffer[*next_tuple];
        child_translators[table_idx].get().SchemaValues().SetValues(
            tuple.Unpack());

        for (int predicate_idx : predicates_per_table[table_idx]) {
          if (evaluated_predicates.contains(predicate_idx)) {
            continue;
          }

          // If there was only one predicate checked via index,
          // we're guaranteed it holds. Otherwise, we checked
          // multiple indexes and so we need to evaluate all
          // predicates again.
          if (index_evaluated_predicates.size() == 1 &&
              index_evaluated_predicates.contains(predicate_idx)) {
            continue;
          }

          auto cond = expr_translator_.Compute(conditions[predicate_idx]);
          proxy::If(
              program_, !static_cast<proxy::Bool&>(*cond),
              [&]() -> std::vector<khir::Value> {
                auto last_tuple = next_tuple->ToPointer();

                proxy::If(
                    program_, budget == 0,
                    [&]() -> std::vector<khir::Value> {
                      // Store last_tuple into global idx array.
                      auto idx_ptr = program_.GetElementPtr(
                          idx_array_type, idx_array, {0, table_idx});
                      program_.StoreI32(idx_ptr, last_tuple->Get());

                      // Set table_ctr to be the current table
                      program_.StoreI32(
                          program_.GetElementPtr(table_ctr_type, table_ctr_ptr,
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
        }

        if (curr + 1 == order.size()) {
          // Valid tuple
        } else {
          available_tables.insert(curr);
          GenerateChildLoops(program, order, curr + 1, evaluated_predicates,
                             predicates_per_table, available_tables);
        }

        return loop.Continue(*next_tuple);
      });
}
*/

void* RecompilingSkinnerJoinTranslator::CompileJoinOrder(
    const std::vector<int>& order) {
  std::cout << "Called in translator: ";
  for (int x : order) {
    std::cout << " " << x;
  }
  std::cout << std::endl;
  return nullptr;
  /*
  auto child_translators = this->Children();
  auto& entry = cache_.GetOrInsert(order);
  if (entry.IsCompiled()) {
    return entry.Func("compute");
  }

  auto& program = entry.ProgramBuilder();
  ForwardDeclare(program);

  program.CreatePublicFunction(program.VoidType(), {}, "compute");
  program.Return();

  entry.Compile();
  return entry.Func("compute"); */
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

  // TODO: implement here

  // 3. Execute join
  proxy::SkinnerJoinExecutor executor(program_);

  auto compile_fn = static_cast<RecompilingJoinTranslator*>(this);
  executor.ExecuteRecompilingJoin(compile_fn);

  // TODO: implement here

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