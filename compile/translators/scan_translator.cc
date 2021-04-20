#include "compile/translators/scan_translator.h"

#include <exception>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "catalog/sql_type.h"
#include "compile/ir_registry.h"
#include "compile/program_builder.h"
#include "compile/proxy/column_data.h"
#include "compile/proxy/loop.h"
#include "compile/translators/operator_translator.h"
#include "plan/scan_operator.h"
#include "util/vector_util.h"

namespace kush::compile {

template <typename T>
ScanTranslator<T>::ScanTranslator(
    const plan::ScanOperator& scan, ProgramBuilder<T>& program,
    std::vector<std::unique_ptr<OperatorTranslator<T>>> children)
    : OperatorTranslator<T>(scan, std::move(children)),
      scan_(scan),
      program_(program) {}

template <typename T>
void ScanTranslator<T>::Produce() {
  const auto& table = scan_.Relation();

  std::vector<std::unique_ptr<proxy::Iterable<T>>> column_data_vars;

  column_data_vars.reserve(scan_.Schema().Columns().size());
  for (const auto& column : scan_.Schema().Columns()) {
    using catalog::SqlType;
    auto type = column.Expr().Type();

    auto path = table[column.Name()].Path();
    switch (type) {
      case SqlType::SMALLINT:
        column_data_vars.push_back(
            std::make_unique<proxy::ColumnData<T, SqlType::SMALLINT>>(program_,
                                                                      path));
        break;
      case SqlType::INT:
        column_data_vars.push_back(
            std::make_unique<proxy::ColumnData<T, SqlType::INT>>(program_,
                                                                 path));
        break;
      case SqlType::BIGINT:
        column_data_vars.push_back(
            std::make_unique<proxy::ColumnData<T, SqlType::BIGINT>>(program_,
                                                                    path));
        break;
      case SqlType::REAL:
        column_data_vars.push_back(
            std::make_unique<proxy::ColumnData<T, SqlType::REAL>>(program_,
                                                                  path));
        break;
      case SqlType::DATE:
        column_data_vars.push_back(
            std::make_unique<proxy::ColumnData<T, SqlType::DATE>>(program_,
                                                                  path));
        break;
      case SqlType::TEXT:
        column_data_vars.push_back(
            std::make_unique<proxy::ColumnData<T, SqlType::TEXT>>(program_,
                                                                  path));
        break;
      case SqlType::BOOLEAN:
        column_data_vars.push_back(
            std::make_unique<proxy::ColumnData<T, SqlType::BOOLEAN>>(program_,
                                                                     path));
        break;
    }
  }

  auto card_var = column_data_vars[0]->Size();

  proxy::IndexLoop<T>(
      program_, [&]() { return proxy::Int32<T>(program_, 0); },
      [&](proxy::Int32<T>& i) { return i < card_var; },
      [&](proxy::Int32<T>& i, auto Continue) {
        this->values_.ResetValues();
        for (auto& col_var : column_data_vars) {
          this->values_.AddVariable((*col_var)[i]);
        }

        if (auto parent = this->Parent()) {
          parent->get().Consume(*this);
        }

        return i + proxy::Int32<T>(program_, 1);
      });

  column_data_vars.clear();
}

template <typename T>
void ScanTranslator<T>::Consume(OperatorTranslator<T>& src) {
  throw std::runtime_error("Scan cannot consume tuples - leaf operator");
}

INSTANTIATE_ON_IR(ScanTranslator);

}  // namespace kush::compile