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

std::unique_ptr<proxy::Iterable> ScanTranslator::ConstructIterable(
    catalog::SqlType type, std::string_view path) {
  using SqlType = catalog::SqlType;
  switch (type) {
    case SqlType::SMALLINT:
      return std::make_unique<proxy::ColumnData<SqlType::SMALLINT>>(program_,
                                                                    path);
    case SqlType::INT:
      return std::make_unique<proxy::ColumnData<SqlType::INT>>(program_, path);
    case SqlType::BIGINT:
      return std::make_unique<proxy::ColumnData<SqlType::BIGINT>>(program_,
                                                                  path);
    case SqlType::REAL:
      return std::make_unique<proxy::ColumnData<SqlType::REAL>>(program_, path);
    case SqlType::DATE:
      return std::make_unique<proxy::ColumnData<SqlType::DATE>>(program_, path);
    case SqlType::TEXT:
      return std::make_unique<proxy::ColumnData<SqlType::TEXT>>(program_, path);
    case SqlType::BOOLEAN:
      return std::make_unique<proxy::ColumnData<SqlType::BOOLEAN>>(program_,
                                                                   path);
  }
}

void ScanTranslator::Open() {
  const auto& table = scan_.Relation();

  const auto& cols = scan_.Schema().Columns();
  auto num_cols = cols.size();
  column_data_vars_.reserve(num_cols);
  null_data_vars_.reserve(num_cols);
  for (const auto& column : cols) {
    auto type = column.Expr().Type();
    auto path = table[column.Name()].Path();

    column_data_vars_.push_back(ConstructIterable(type, path));

    if (table[column.Name()].Nullable()) {
      null_data_vars_.push_back(ConstructIterable(
          catalog::SqlType::BOOLEAN, table[column.Name()].NullPath()));
    } else {
      null_data_vars_.push_back(nullptr);
    }
  }

  size_var_ = column_data_vars_[0]->Size().ToPointer();
}

proxy::SQLValue ScanTranslator::Get(int col_idx, proxy::Int32 tuple_idx) {
  const auto& cols = scan_.Schema().Columns();
  auto& column_data = *column_data_vars_[col_idx];
  auto type = cols[col_idx].Expr().Type();
  if (null_data_vars_[col_idx] == nullptr) {
    return proxy::SQLValue(column_data[tuple_idx], type,
                           proxy::Bool(program_, false));
  } else {
    auto& null_data = *null_data_vars_[col_idx];
    auto null = null_data[tuple_idx];
    return proxy::SQLValue(column_data[tuple_idx], type,
                           static_cast<proxy::Bool&>(*null));
  }
}

void ScanTranslator::Produce() {
  Open();

  proxy::Loop loop(
      program_,
      [&](auto& loop) { loop.AddLoopVariable(proxy::Int32(program_, 0)); },
      [&](auto& loop) {
        auto i = loop.template GetLoopVariable<proxy::Int32>(0);
        return i < *size_var_;
      },
      [&](auto& loop) {
        auto i = loop.template GetLoopVariable<proxy::Int32>(0);

        this->values_.ResetValues();
        for (int k = 0; k < column_data_vars_.size(); k++) {
          this->values_.AddVariable(Get(k, i));
        }

        if (auto parent = this->Parent()) {
          parent->get().Consume(*this);
        }

        return loop.Continue(i + 1);
      });

  Reset();
}

void ScanTranslator::Reset() {
  for (auto& col : column_data_vars_) {
    col->Reset();
  }
  for (auto& col : null_data_vars_) {
    if (col != nullptr) {
      col->Reset();
    }
  }
}

proxy::Int32 ScanTranslator::Size() { return *size_var_; }

void ScanTranslator::Consume(OperatorTranslator& src) {
  throw std::runtime_error("Scan cannot consume tuples - leaf operator");
}

void ScanTranslator::Produce(int col_idx,
                             std::function<void(const proxy::Int32& tuple_idx,
                                                const proxy::SQLValue& value)>
                                 handler) {
  proxy::Loop loop(
      program_,
      [&](auto& loop) { loop.AddLoopVariable(proxy::Int32(program_, 0)); },
      [&](auto& loop) {
        auto i = loop.template GetLoopVariable<proxy::Int32>(0);
        return i < *size_var_;
      },
      [&](auto& loop) {
        auto i = loop.template GetLoopVariable<proxy::Int32>(0);

        handler(i, Get(col_idx, i));

        return loop.Continue(i + 1);
      });
}

}  // namespace kush::compile