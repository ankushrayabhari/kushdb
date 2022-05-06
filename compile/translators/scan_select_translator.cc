#include "compile/translators/scan_select_translator.h"

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"

#include "compile/proxy/column_data.h"
#include "compile/proxy/control_flow/if.h"
#include "compile/proxy/control_flow/loop.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "compile/translators/predicate_column_collector.h"
#include "khir/program_builder.h"
#include "plan/operator/scan_select_operator.h"

namespace kush::compile {

ScanSelectTranslator::ScanSelectTranslator(
    const plan::ScanSelectOperator& scan_select, khir::ProgramBuilder& program)
    : OperatorTranslator(scan_select, {}),
      scan_select_(scan_select),
      program_(program),
      expr_translator_(program, *this) {}

std::unique_ptr<proxy::DiskMaterializedBuffer>
ScanSelectTranslator::GenerateBuffer() {
  const auto& table = scan_select_.Relation();
  const auto& cols = scan_select_.ScanSchema().Columns();
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
            std::make_unique<proxy::ColumnData<TypeId::SMALLINT>>(program_,
                                                                  path, type));
        break;
      case TypeId::INT:
        column_data.push_back(std::make_unique<proxy::ColumnData<TypeId::INT>>(
            program_, path, type));
        break;
      case TypeId::ENUM:
        column_data.push_back(std::make_unique<proxy::ColumnData<TypeId::ENUM>>(
            program_, path, type));
        break;
      case TypeId::BIGINT:
        column_data.push_back(
            std::make_unique<proxy::ColumnData<TypeId::BIGINT>>(program_, path,
                                                                type));
        break;
      case TypeId::REAL:
        column_data.push_back(std::make_unique<proxy::ColumnData<TypeId::REAL>>(
            program_, path, type));
        break;
      case TypeId::DATE:
        column_data.push_back(std::make_unique<proxy::ColumnData<TypeId::DATE>>(
            program_, path, type));
        break;
      case TypeId::TEXT:
        column_data.push_back(std::make_unique<proxy::ColumnData<TypeId::TEXT>>(
            program_, path, type));
        break;
      case TypeId::BOOLEAN:
        column_data.push_back(
            std::make_unique<proxy::ColumnData<TypeId::BOOLEAN>>(program_, path,
                                                                 type));
        break;
    }

    if (table[column.Name()].Nullable()) {
      null_data.push_back(std::make_unique<proxy::ColumnData<TypeId::BOOLEAN>>(
          program_, table[column.Name()].NullPath(), catalog::Type::Boolean()));
    } else {
      null_data.push_back(nullptr);
    }
  }

  return std::make_unique<proxy::DiskMaterializedBuffer>(
      program_, std::move(column_data), std::move(null_data));
}

void ScanSelectTranslator::Produce() {
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

        const auto& scan_schema_columns = scan_select_.ScanSchema().Columns();
        for (int i = 0; i < scan_schema_columns.size(); i++) {
          const auto& type = scan_schema_columns[i].Expr().Type();
          switch (type.type_id) {
            case catalog::TypeId::SMALLINT:
              this->virtual_values_.AddVariable(proxy::SQLValue(
                  proxy::Int16(program_, 0), proxy::Bool(program_, false)));
              break;
            case catalog::TypeId::INT:
              this->virtual_values_.AddVariable(proxy::SQLValue(
                  proxy::Int32(program_, 0), proxy::Bool(program_, false)));
              break;
            case catalog::TypeId::DATE:
              this->virtual_values_.AddVariable(proxy::SQLValue(
                  proxy::Date(program_, runtime::Date::DateBuilder(2000, 1, 1)),
                  proxy::Bool(program_, false)));
              break;
            case catalog::TypeId::BIGINT:
              this->virtual_values_.AddVariable(proxy::SQLValue(
                  proxy::Int64(program_, 0), proxy::Bool(program_, false)));
              break;
            case catalog::TypeId::BOOLEAN:
              this->virtual_values_.AddVariable(proxy::SQLValue(
                  proxy::Bool(program_, false), proxy::Bool(program_, false)));
              break;
            case catalog::TypeId::REAL:
              this->virtual_values_.AddVariable(proxy::SQLValue(
                  proxy::Float64(program_, 0), proxy::Bool(program_, false)));
              break;
            case catalog::TypeId::TEXT:
              this->virtual_values_.AddVariable(
                  proxy::SQLValue(proxy::String::Global(program_, ""),
                                  proxy::Bool(program_, false)));
              break;
            case catalog::TypeId::ENUM:
              this->virtual_values_.AddVariable(
                  proxy::SQLValue(proxy::Enum(program_, type.enum_id, -1),
                                  proxy::Bool(program_, false)));
              break;
          }
        }

        absl::flat_hash_set<int> loaded_cols;

        for (auto condition : scan_select_.Filters()) {
          ScanSelectPredicateColumnCollector collector;
          condition.get().Accept(collector);
          for (auto col : collector.PredicateColumns()) {
            auto col_idx = col.get().GetColumnIdx();
            if (!loaded_cols.contains(col_idx)) {
              loaded_cols.insert(col_idx);
              this->virtual_values_.SetValue(
                  col_idx, materialized_buffer->Get(i, col_idx));
            }
          }

          auto value = expr_translator_.Compute(condition.get());
          proxy::If(program_, value.IsNull(), [&]() { loop.Continue(i + 1); });
          proxy::If(program_, NOT, static_cast<proxy::Bool&>(value.Get()),
                    [&]() { loop.Continue(i + 1); });
        }

        auto num_cols = scan_schema_columns.size();
        for (int col_idx = 0; col_idx < num_cols; col_idx++) {
          if (!loaded_cols.contains(col_idx)) {
            this->virtual_values_.SetValue(
                col_idx, materialized_buffer->Get(i, col_idx));
          }
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

void ScanSelectTranslator::Consume(OperatorTranslator& src) {
  throw std::runtime_error("Scan cannot consume tuples - leaf operator");
}

}  // namespace kush::compile