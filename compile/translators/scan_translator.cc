#include "compile/translators/scan_translator.h"

#include <exception>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"

#include "catalog/sql_type.h"
#include "compile/proxy/column_data.h"
#include "compile/proxy/control_flow/loop.h"
#include "compile/proxy/disk_column_index.h"
#include "compile/proxy/value/ir_value.h"
#include "compile/proxy/value/sql_value.h"
#include "compile/translators/operator_translator.h"
#include "khir/program_builder.h"
#include "plan/operator/scan_operator.h"
#include "util/vector_util.h"

namespace kush::compile {

ScanTranslator::ScanTranslator(const plan::ScanOperator& scan,
                               khir::ProgramBuilder& program,
                               execution::QueryState& state)
    : OperatorTranslator(scan, {}),
      scan_(scan),
      program_(program),
      state_(state) {}

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
    using catalog::TypeId;
    auto type = column.Expr().Type();
    auto path = table[column.Name()].Path();
    switch (type.type_id) {
      case TypeId::SMALLINT:
        column_data.push_back(
            std::make_unique<proxy::ColumnData<TypeId::SMALLINT>>(
                program_, state_, path, type));
        break;
      case TypeId::INT:
        column_data.push_back(std::make_unique<proxy::ColumnData<TypeId::INT>>(
            program_, state_, path, type));
        break;
      case TypeId::ENUM:
        column_data.push_back(std::make_unique<proxy::ColumnData<TypeId::ENUM>>(
            program_, state_, path, type));
        break;
      case TypeId::BIGINT:
        column_data.push_back(
            std::make_unique<proxy::ColumnData<TypeId::BIGINT>>(
                program_, state_, path, type));
        break;
      case TypeId::REAL:
        column_data.push_back(std::make_unique<proxy::ColumnData<TypeId::REAL>>(
            program_, state_, path, type));
        break;
      case TypeId::DATE:
        column_data.push_back(std::make_unique<proxy::ColumnData<TypeId::DATE>>(
            program_, state_, path, type));
        break;
      case TypeId::TEXT:
        column_data.push_back(std::make_unique<proxy::ColumnData<TypeId::TEXT>>(
            program_, state_, path, type));
        break;
      case TypeId::BOOLEAN:
        column_data.push_back(
            std::make_unique<proxy::ColumnData<TypeId::BOOLEAN>>(
                program_, state_, path, type));
        break;
    }

    if (table[column.Name()].Nullable()) {
      null_data.push_back(std::make_unique<proxy::ColumnData<TypeId::BOOLEAN>>(
          program_, state_, table[column.Name()].NullPath(),
          catalog::Type::Boolean()));
    } else {
      null_data.push_back(nullptr);
    }
  }

  return std::make_unique<proxy::DiskMaterializedBuffer>(
      program_, std::move(column_data), std::move(null_data));
}

bool ScanTranslator::HasIndex(int col_idx) {
  const auto& table = scan_.Relation();
  const auto& cols = scan_.Schema().Columns();
  auto col_name = cols[col_idx].Name();
  return table[col_name].HasIndex();
}

std::unique_ptr<proxy::ColumnIndex> ScanTranslator::GenerateIndex(int col_idx) {
  const auto& table = scan_.Relation();
  const auto& cols = scan_.Schema().Columns();
  const auto& column = cols[col_idx];
  auto type = column.Expr().Type();
  auto path = table[column.Name()].IndexPath();
  using catalog::TypeId;
  switch (type.type_id) {
    case TypeId::SMALLINT:
      return std::make_unique<proxy::DiskColumnIndex<TypeId::SMALLINT>>(
          program_, state_, path);
    case TypeId::INT:
      return std::make_unique<proxy::DiskColumnIndex<TypeId::INT>>(
          program_, state_, path);
    case TypeId::ENUM:
      return std::make_unique<proxy::DiskColumnIndex<TypeId::ENUM>>(
          program_, state_, path);
    case TypeId::BIGINT:
      return std::make_unique<proxy::DiskColumnIndex<TypeId::BIGINT>>(
          program_, state_, path);
    case TypeId::REAL:
      return std::make_unique<proxy::DiskColumnIndex<TypeId::REAL>>(
          program_, state_, path);
    case TypeId::DATE:
      return std::make_unique<proxy::DiskColumnIndex<TypeId::DATE>>(
          program_, state_, path);
    case TypeId::TEXT:
      return std::make_unique<proxy::DiskColumnIndex<TypeId::TEXT>>(
          program_, state_, path);
    case TypeId::BOOLEAN:
      return std::make_unique<proxy::DiskColumnIndex<TypeId::BOOLEAN>>(
          program_, state_, path);
  }
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