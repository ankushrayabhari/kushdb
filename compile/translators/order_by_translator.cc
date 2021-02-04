#include "compile/translators/order_by_translator.h"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "catalog/sql_type.h"
#include "compile/ir_registry.h"
#include "compile/proxy/vector.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "plan/order_by_operator.h"

namespace kush::compile {

template <typename T>
OrderByTranslator<T>::OrderByTranslator(
    const plan::OrderByOperator& order_by, ProgramBuilder<T>& program,
    std::vector<std::unique_ptr<OperatorTranslator<T>>> children)
    : OperatorTranslator(std::move(children)),
      order_by_(order_by),
      program_(program),
      expr_translator_(program, *this) {}

template <typename T>
void OrderByTranslator<T>::Produce() {
  // include every child column inside the struct
  std::vector<std::reference_wrapper<ProgramBuilder<T>::Type>> field_types;
  const auto& child_schema = order_by_.Child().Schema().Columns();
  for (const auto& col : child_schema) {
    switch (col.Expr().Type()) {
      case catalog::SqlType::SMALLINT:
        field_types.push_back(program_.I16Type());
        break;
      case catalog::SqlType::INT:
        field_types.push_back(program_.I32Type());
        break;
      case catalog::SqlType::BIGINT:
        field_types.push_back(program_.I64Type());
        break;
      case catalog::SqlType::REAL:
        field_types.push_back(program_.F64Type());
        break;
      case catalog::SqlType::DATE:
        field_types.push_back(program_.I64Type());
        break;
      case catalog::SqlType::TEXT:
        throw std::runtime_error("Text not supported at the moment.");
        break;
      case catalog::SqlType::BOOLEAN:
        field_types.push_back(program_.I8Type());
        break;
    }
  }
  auto& packed_type = program_.StructType(field_types);

  /*
    // generate sort function
    auto comp_fn = program.GenerateVariable();
    program.fout << " auto " << comp_fn << " = [](const " << packed_struct_id_
                 << "& p1, const " << packed_struct_id_ << "& p2) -> bool{\n";

    const auto sort_keys = order_by_.KeyExprs();
    const auto& ascending = order_by_.Ascending();
    for (int i = 0; i < sort_keys.size(); i++) {
      int field_idx = sort_keys[i].get().GetColumnIdx();
      const auto& [field, _] = packed_field_type_[field_idx];
      auto asc = ascending[i];

      if (i > 0) {
        int last_field_idx = sort_keys[i - 1].get().GetColumnIdx();
        const auto& [last_field, _] = packed_field_type_[last_field_idx];

        program.fout << "if (p1." << last_field << (" != ") << "p2." <<
    last_field
                     << ") return false;\n";
      }

      program.fout << "if (p1." << field << (asc ? " < " : " > ") << "p2."
                   << field << ") return true;\n";
    }
    program.fout << "return false;\n};\n";
  */

  // init vector
  proxy::Vector<T> buffer(program_, packed_type);

  // populate vector
  Child().Produce();

  // sort
  buffer.Sort();

  // output
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

    program.fout << "auto " << var << " = "
                 << expr_translator_.Compute(column.Expr())->Get();
    program.fout << ";\n";

    values_.AddVariable(var, type);
  }

  if (auto parent = Parent()) {
    parent->get().Consume(*this);
  }
}

template <typename T>
void OrderByTranslator<T>::Consume(OperatorTranslator<T>& src) {
  /*
  // append elements to array
  auto& program = context_.Program();
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
  */
  auto& ptr = buffer.Append();
  const auto& child_vars = Child().GetValues().Values();
  for (int i = 0; i < child_vars.size(); i++) {
    auto& data_ptr = program_.GetElementPtr(
        struct_ptr_type, ptr, {program_.ConstI32(0), program_.ConstI32(i)})
  }
}

}  // namespace kush::compile