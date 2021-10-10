#include "compile/translators/scan_translator.h"

#include <exception>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"

#include "catalog/sql_type.h"
#include "compile/proxy/column_data.h"
#include "compile/proxy/control_flow/loop.h"
#include "compile/proxy/value/ir_value.h"
#include "compile/proxy/value/sql_value.h"
#include "compile/translators/operator_translator.h"
#include "khir/program_builder.h"
#include "plan/scan_operator.h"
#include "util/vector_util.h"

namespace kush::compile {

ScanTranslator::ScanTranslator(
    const plan::ScanOperator& scan, khir::ProgramBuilder& program,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(scan, std::move(children)),
      scan_(scan),
      program_(program) {}

std::unique_ptr<proxy::DiskMaterializedBuffer>
ScanTranslator::GenerateBuffer() {
  const auto& table = scan_.Relation();
  const auto& cols = scan_.Schema().Columns();
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

void ScanTranslator::Produce() {
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

        this->values_.SetValues((*materialized_buffer)[i]);

        if (auto parent = this->Parent()) {
          parent->get().Consume(*this);
        }

        return loop.Continue(i + 1);
      });

  materialized_buffer->Reset();
}

void ScanTranslator::Consume(OperatorTranslator& src) {
  throw std::runtime_error("Scan cannot consume tuples - leaf operator");
}

}  // namespace kush::compile