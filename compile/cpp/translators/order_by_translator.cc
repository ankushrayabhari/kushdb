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

  // declare packing struct of each input row
  packed_struct_id_ = program.GenerateVariable();
  program.fout << " struct " << packed_struct_id_ << " {\n";

  const auto& child_schema = order_by_.Child().Schema().Columns();
  const auto sort_keys = order_by_.KeyExprs();
  const auto& ascending = order_by_.Ascending();

  // include every child column inside the struct
  for (const auto& col : child_schema) {
    auto field = program.GenerateVariable();
    auto type = SqlTypeToRuntimeType(col.Expr().Type());

    packed_field_type_.emplace_back(field, type);
    program.fout << type << " " << field << ";\n";
  }

  program.fout << "};\n";

  // generate sort function
  auto comp_fn = program.GenerateVariable();
  program.fout << " auto " << comp_fn << " = [](const " << packed_struct_id_
               << "& p1, const " << packed_struct_id_ << "& p2) -> bool{\n";

  program.fout << "return ";
  for (int i = 0; i < sort_keys.size(); i++) {
    int field_idx = sort_keys[i].get().GetColumnIdx();
    const auto& [field, _] = packed_field_type_[field_idx];
    auto asc = ascending[field_idx];

    if (i > 0) {
      program.fout << " && ";
    }

    program.fout << "p1." << field << (asc ? " < " : " > ") << "p2." << field
                 << "\n";
  }
  program.fout << ";\n};\n";

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
  auto child_vars = Child().GetValues().Values();
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
  auto& program = context_.Program();
  const auto& child_vars = Child().GetValues().Values();
  program.fout << buffer_var_ << ".push_back(" << packed_struct_id_ << "{";
  bool first = true;
  for (const auto& [variable, type] : child_vars) {
    if (first) {
      first = false;
    } else {
      program.fout << ",";
    }
    program.fout << variable;
  }
  program.fout << "});\n";
}

}  // namespace kush::compile::cpp