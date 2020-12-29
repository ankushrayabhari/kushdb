#include "compile/cpp/translators/order_by_translator.h"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "compile/cpp/cpp_compilation_context.h"
#include "compile/cpp/translators/expression_translator.h"
#include "compile/cpp/translators/operator_translator.h"
#include "compile/cpp/types.h"
#include "plan/order_by_operator.h"

namespace kush::compile::cpp {

OrderByTranslator::OrderByTranslator(
    const plan::OrderByOperator& order_by, CppCompilationContext& context,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(std::move(children)),
      order_by_(order_by),
      context_(context),
      expr_translator_(context, *this) {}

void OrderByTranslator::Produce() {
  auto& program = context_.Program();

  Child().Produce();

  // declare packing struct of each input row
  packed_struct_id_ = program.GenerateVariable();
  program.fout << " struct " << packed_struct_id_ << " {\n";

  const auto& child_vars = Child().GetValues().Values();
  auto sort_keys = order_by_.KeyExprs();
  const auto& ascending = order_by_.Ascending();

  // include every child column inside the struct
  for (const auto& [var, type] : child_vars) {
    auto field = program.GenerateVariable();
    packed_field_type_.emplace_back(field, type);
    program.fout << type << " " << field << ";\n";
  }

  program.fout << "};\n";

  // generate sort function
  auto comp_fn = program.GenerateVariable();
  program.fout << " auto " << comp_fn << " = [](const " << packed_struct_id_
               << "& p1, const " << packed_struct_id_ << "& p2){ -> bool\n";

  for (int i = 0; i < sort_keys.size(); i++) {
    int field_idx = sort_keys[i].get().GetColumnIdx();
    const auto& [field, type] = packed_field_type_[field_idx];
    auto asc = ascending[i];

    program.fout << "return p1." << field << (asc ? "<" : ">") << " p2."
                 << field << ";\n";
  }
  program.fout << "};\n";

  // init vector
  buffer_var_ = program.GenerateVariable();
  program.fout << "std::vector<" << packed_struct_id_ << "> " << buffer_var_
               << ";\n";

  // populate vector
  Child().Produce();

  // sort
  program.fout << "std::sort(" << buffer_var_ << ".begin(), " << buffer_var_
               << ".end(), " << comp_fn << ");\n";

  // output
  auto loop_var = program.GenerateVariable();
  program.fout << "for (int " << loop_var << " = 0; " << loop_var << " < "
               << buffer_var_ << ".size(); " << loop_var << "++){\n";

  // unpack struct
  for (int i = 0; i < packed_field_type_.size(); i++) {
    const auto& [field, type] = packed_field_type_[i];
    const auto& [variable, _] = child_vars[i];

    program.fout << type << "& " << variable << " = " << buffer_var_ << "["
                 << loop_var << "]." << field << ";\n";
  }

  // reuse child variables
  for (const auto& column : order_by_.Schema().Columns()) {
    auto var = program.GenerateVariable();
    auto type = SqlTypeToRuntimeType(column.Expr().Type());

    program.fout << "auto& " << var << " = ";
    expr_translator_.Produce(column.Expr());
    program.fout << ";\n";

    values_.AddVariable(var, type);
  }

  if (auto parent = Parent()) {
    parent->get().Consume(*this);
  }

  program.fout << "}\n";
}

void OrderByTranslator::Consume(OperatorTranslator& src) {
  // append elements to array
}

}  // namespace kush::compile::cpp