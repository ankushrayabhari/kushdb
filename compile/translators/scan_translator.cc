#include "compile/translators/scan_translator.h"

#include <exception>
#include <string>
#include <vector>

#include "compile/compilation_context.h"
#include "compile/cpp_program.h"
#include "compile/translators/operator_translator.h"
#include "compile/types.h"
#include "plan/scan_operator.h"

namespace kush::compile {

ScanTranslator::ScanTranslator(
    plan::ScanOperator& scan, CompilationContext& context,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(std::move(children)), scan_(scan), context_(context) {}

void ScanTranslator::Produce() {
  const auto& table = context_.Catalog()[scan_.Relation()];
  auto& program = context_.Program();

  std::vector<std::string> column_vars;
  std::string card_var;

  for (const auto& column : scan_.Schema().Columns()) {
    auto var = program.GenerateVariable();
    auto type = SqlTypeToRuntimeType(column.Type());
    auto path = table[std::string(column.Name())].Path();
    column_vars.push_back(var);

    // declare the column
    program.fout << "const kush::data::ColumnData<" << type << "> " << var
                 << "(\"" << path << "\");\n";

    if (card_var.empty()) {
      card_var = var + ".Size()";
    }
  }

  std::string loop_var = program.GenerateVariable();
  program.fout << "for (uint32_t " << loop_var << " = 0; " << loop_var << " < "
               << card_var << "; " << loop_var << "++) {\n";

  for (int i = 0; i < column_vars.size(); i++) {
    const auto& column = scan_.Schema().Columns()[i];
    std::string type = SqlTypeToRuntimeType(column.Type());
    std::string column_var = column_vars[i];
    std::string value_var = program.GenerateVariable();

    program.fout << type << " " << value_var << " = " << column_var << "["
                 << loop_var << "];\n";

    values_.AddVariable(value_var, type);
  }

  if (auto parent = Parent()) {
    parent->get().Consume(*this);
  }

  program.fout << "}\n";
}

void ScanTranslator::Consume(OperatorTranslator& src) {
  throw std::runtime_error("Scan cannot consume tuples - leaf operator");
}

}  // namespace kush::compile