#include "compile/translators/scan_translator.h"

#include <exception>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"

#include "catalog/sql_type.h"
#include "compile/proxy/column_data.h"
#include "compile/proxy/loop.h"
#include "compile/proxy/printer.h"
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

  column_data_vars.reserve(scan_.Schema().Columns().size());
  for (const auto& column : scan_.Schema().Columns()) {
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
        for (auto& col_var : column_data_vars) {
          this->values_.AddVariable((*col_var)[i]);
        }

        if (auto parent = this->Parent()) {
          parent->get().Consume(*this);
        }

        return loop.Continue(i + 1);
      });

  proxy::Printer printer(program_);
  printer.Print(loop.template GetLoopVariable<proxy::Int32>(0));
  printer.PrintNewline();

  for (auto& col : column_data_vars) {
    col->Reset();
  }
}

void ScanTranslator::Consume(OperatorTranslator& src) {
  throw std::runtime_error("Scan cannot consume tuples - leaf operator");
}

}  // namespace kush::compile