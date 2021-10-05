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

void ScanTranslator::Produce() {
  const auto& table = scan_.Relation();

  std::vector<std::unique_ptr<proxy::Iterable>> column_data_vars;
  std::vector<std::unique_ptr<proxy::Iterable>> null_data_vars;
  const auto& cols = scan_.Schema().Columns();
  auto num_cols = cols.size();
  column_data_vars.reserve(num_cols);
  null_data_vars.reserve(num_cols);
  for (const auto& column : cols) {
    using catalog::SqlType;
    auto type = column.Expr().Type();
    auto path = table[column.Name()].Path();
    switch (type) {
      case SqlType::SMALLINT:
        column_data_vars.push_back(
            std::make_unique<proxy::ColumnData<SqlType::SMALLINT>>(program_,
                                                                   path));
        break;
      case SqlType::INT:
        column_data_vars.push_back(
            std::make_unique<proxy::ColumnData<SqlType::INT>>(program_, path));
        break;
      case SqlType::BIGINT:
        column_data_vars.push_back(
            std::make_unique<proxy::ColumnData<SqlType::BIGINT>>(program_,
                                                                 path));
        break;
      case SqlType::REAL:
        column_data_vars.push_back(
            std::make_unique<proxy::ColumnData<SqlType::REAL>>(program_, path));
        break;
      case SqlType::DATE:
        column_data_vars.push_back(
            std::make_unique<proxy::ColumnData<SqlType::DATE>>(program_, path));
        break;
      case SqlType::TEXT:
        column_data_vars.push_back(
            std::make_unique<proxy::ColumnData<SqlType::TEXT>>(program_, path));
        break;
      case SqlType::BOOLEAN:
        column_data_vars.push_back(
            std::make_unique<proxy::ColumnData<SqlType::BOOLEAN>>(program_,
                                                                  path));
        break;
    }

    if (table[column.Name()].Nullable()) {
      null_data_vars.push_back(nullptr);
    } else {
      null_data_vars.push_back(
          std::make_unique<proxy::ColumnData<SqlType::BOOLEAN>>(
              program_, table[column.Name()].NullPath()));
    }
  }

  auto card_var = column_data_vars[0]->Size();

  proxy::Loop loop(
      program_,
      [&](auto& loop) { loop.AddLoopVariable(proxy::Int32(program_, 0)); },
      [&](auto& loop) {
        auto i = loop.template GetLoopVariable<proxy::Int32>(0);
        return i < card_var;
      },
      [&](auto& loop) {
        auto i = loop.template GetLoopVariable<proxy::Int32>(0);

        this->values_.ResetValues();
        for (int k = 0; k < column_data_vars.size(); k++) {
          auto& column_data = *column_data_vars[k];
          auto type = cols[k].Expr().Type();
          if (null_data_vars[k] == nullptr) {
            this->values_.AddVariable(proxy::SQLValue(
                column_data[i], type, proxy::Bool(program_, false)));
          } else {
            auto null = null_data_vars[k]->operator[](i);
            this->values_.AddVariable(proxy::SQLValue(
                column_data[i], type, static_cast<proxy::Bool&>(*null)));
          }
        }

        if (auto parent = this->Parent()) {
          parent->get().Consume(*this);
        }

        return loop.Continue(i + 1);
      });

  for (auto& col : column_data_vars) {
    col->Reset();
  }
}

void ScanTranslator::Consume(OperatorTranslator& src) {
  throw std::runtime_error("Scan cannot consume tuples - leaf operator");
}

}  // namespace kush::compile