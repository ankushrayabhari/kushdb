#include "compile/translators/scan_translator.h"

#include <exception>
#include <string>
#include <vector>

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
    : OperatorTranslator<T>(std::move(children)),
      scan_(scan),
      program_(program) {}

template <typename T>
void ScanTranslator<T>::Produce() {
  const auto& table = scan_.Relation();

  std::vector<std::unique_ptr<proxy::Iterable<T>>> column_data_vars;
  std::unique_ptr<proxy::UInt32<T>> card_var;

  column_data_vars.reserve(scan_.Schema().Columns().size());
  for (const auto& column : scan_.Schema().Columns()) {
    auto path = table[column.Name()].Path();
    switch (column.Expr().Type()) {
      case catalog::SqlType::SMALLINT: {
        auto var =
            std::make_unique<proxy::ColumnData<T, catalog::SqlType::SMALLINT>>(
                program_, path);
        if (card_var == nullptr) {
          card_var = var->Size();
        }
        column_data_vars.push_back(std::move(var));
        break;
      }

      case catalog::SqlType::INT: {
        auto var =
            std::make_unique<proxy::ColumnData<T, catalog::SqlType::INT>>(
                program_, path);
        if (card_var == nullptr) {
          card_var = var->Size();
        }
        column_data_vars.push_back(std::move(var));
        break;
      }

      case catalog::SqlType::BIGINT: {
        auto var =
            std::make_unique<proxy::ColumnData<T, catalog::SqlType::BIGINT>>(
                program_, path);
        if (card_var == nullptr) {
          card_var = var->Size();
        }
        column_data_vars.push_back(std::move(var));
        break;
      }

      case catalog::SqlType::REAL: {
        auto var =
            std::make_unique<proxy::ColumnData<T, catalog::SqlType::REAL>>(
                program_, path);
        if (card_var == nullptr) {
          card_var = var->Size();
        }
        column_data_vars.push_back(std::move(var));
        break;
      }

      case catalog::SqlType::DATE: {
        auto var =
            std::make_unique<proxy::ColumnData<T, catalog::SqlType::DATE>>(
                program_, path);
        if (card_var == nullptr) {
          card_var = var->Size();
        }
        column_data_vars.push_back(std::move(var));
        break;
      }

      case catalog::SqlType::TEXT:
        throw std::runtime_error("Text type unsupported at the moment.");
        break;

      case catalog::SqlType::BOOLEAN: {
        auto var =
            std::make_unique<proxy::ColumnData<T, catalog::SqlType::BOOLEAN>>(
                program_, path);
        if (card_var == nullptr) {
          card_var = var->Size();
        }
        column_data_vars.push_back(std::move(var));
        break;
      }
    }
  }

  proxy::Loop<T>(
      program_,
      [&]() {
        return util::MakeVector<std::unique_ptr<proxy::Value<T>>>(
            std::make_unique<proxy::UInt32<T>>(program_, 0));
      },
      [&](proxy::Loop<T>& loop) {
        auto idx = loop.template LoopVariable<proxy::UInt32<T>>(0);
        return *idx < *card_var;
      },
      [&](proxy::Loop<T>& loop) {
        auto idx = loop.template LoopVariable<proxy::UInt32<T>>(0);

        for (auto& col_var : column_data_vars) {
          this->values_.AddVariable((*col_var)[*idx]);
        }

        if (auto parent = this->Parent()) {
          parent->get().Consume(*this);
        }

        return util::MakeVector<std::unique_ptr<proxy::Value<T>>>(
            *idx + *std::make_unique<proxy::UInt32<T>>(program_, 1));
      });
}

template <typename T>
void ScanTranslator<T>::Consume(OperatorTranslator<T>& src) {
  throw std::runtime_error("Scan cannot consume tuples - leaf operator");
}

INSTANTIATE_ON_IR(ScanTranslator);

}  // namespace kush::compile