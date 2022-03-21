#include "compile/translators/none_skinner_scan_select_translator.h"

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"

#include "compile/proxy/column_data.h"
#include "compile/proxy/control_flow/if.h"
#include "compile/proxy/control_flow/loop.h"
#include "compile/proxy/skinner_scan_select_executor.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "compile/translators/predicate_column_collector.h"
#include "khir/program_builder.h"
#include "plan/operator/skinner_scan_select_operator.h"

namespace kush::compile {

NoneSkinnerScanSelectTranslator::NoneSkinnerScanSelectTranslator(
    const plan::SkinnerScanSelectOperator& scan_select,
    khir::ProgramBuilder& program)
    : OperatorTranslator(scan_select, {}),
      scan_select_(scan_select),
      program_(program),
      expr_translator_(program, *this) {}

std::unique_ptr<proxy::DiskMaterializedBuffer>
NoneSkinnerScanSelectTranslator::GenerateBuffer() {
  const auto& table = scan_select_.Relation();
  const auto& cols = scan_select_.ScanSchema().Columns();
  auto num_cols = cols.size();

  std::vector<std::unique_ptr<proxy::Iterable>> column_data;
  std::vector<std::unique_ptr<proxy::Iterable>> null_data;
  column_data.reserve(num_cols);
  null_data.reserve(num_cols);
  for (const auto& column : cols) {
    using catalog::SqlType;
    auto type = column.Expr().Type();
    auto path = table[column.Name()].Path();
    switch (type) {
      case SqlType::SMALLINT:
        column_data.push_back(
            std::make_unique<proxy::ColumnData<SqlType::SMALLINT>>(program_,
                                                                   path));
        break;
      case SqlType::INT:
        column_data.push_back(
            std::make_unique<proxy::ColumnData<SqlType::INT>>(program_, path));
        break;
      case SqlType::BIGINT:
        column_data.push_back(
            std::make_unique<proxy::ColumnData<SqlType::BIGINT>>(program_,
                                                                 path));
        break;
      case SqlType::REAL:
        column_data.push_back(
            std::make_unique<proxy::ColumnData<SqlType::REAL>>(program_, path));
        break;
      case SqlType::DATE:
        column_data.push_back(
            std::make_unique<proxy::ColumnData<SqlType::DATE>>(program_, path));
        break;
      case SqlType::TEXT:
        column_data.push_back(
            std::make_unique<proxy::ColumnData<SqlType::TEXT>>(program_, path));
        break;
      case SqlType::BOOLEAN:
        column_data.push_back(
            std::make_unique<proxy::ColumnData<SqlType::BOOLEAN>>(program_,
                                                                  path));
        break;
    }

    if (table[column.Name()].Nullable()) {
      null_data.push_back(std::make_unique<proxy::ColumnData<SqlType::BOOLEAN>>(
          program_, table[column.Name()].NullPath()));
    } else {
      null_data.push_back(nullptr);
    }
  }

  return std::make_unique<proxy::DiskMaterializedBuffer>(
      program_, std::move(column_data), std::move(null_data));
}

void NoneSkinnerScanSelectTranslator::Produce() {
  auto materialized_buffer = GenerateBuffer();
  materialized_buffer->Init();

  auto cardinality = materialized_buffer->Size();
  proxy::Loop loop(
      program_,
      [&](auto& loop) { loop.AddLoopVariable(proxy::Int32(program_, 0)); },
      [&](auto& loop) {
        auto i = loop.template GetLoopVariable<proxy::Int32>(0);
        return i < cardinality;
      },
      [&](auto& loop) {
        auto i = loop.template GetLoopVariable<proxy::Int32>(0);

        this->virtual_values_.SetValues((*materialized_buffer)[i]);

        for (auto condition : scan_select_.Filters()) {
          auto value = expr_translator_.Compute(condition.get());
          proxy::If(program_, value.IsNull(), [&]() { loop.Continue(i + 1); });
          proxy::If(program_, !static_cast<proxy::Bool&>(value.Get()),
                    [&]() { loop.Continue(i + 1); });
        }

        this->values_.ResetValues();
        for (const auto& column : scan_select_.Schema().Columns()) {
          this->values_.AddVariable(expr_translator_.Compute(column.Expr()));
        }

        if (auto parent = this->Parent()) {
          parent->get().Consume(*this);
        }

        return loop.Continue(i + 1);
      });

  materialized_buffer->Reset();
}

void NoneSkinnerScanSelectTranslator::Consume(OperatorTranslator& src) {
  throw std::runtime_error("Scan cannot consume tuples - leaf operator");
}

}  // namespace kush::compile