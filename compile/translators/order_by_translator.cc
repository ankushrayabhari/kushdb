#include "compile/translators/order_by_translator.h"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "catalog/sql_type.h"
#include "compile/ir_registry.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/float.h"
#include "compile/proxy/int.h"
#include "compile/proxy/loop.h"
#include "compile/proxy/type.h"
#include "compile/proxy/vector.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "plan/order_by_operator.h"

namespace kush::compile {

template <typename T>
OrderByTranslator<T>::OrderByTranslator(
    const plan::OrderByOperator& order_by, ProgramBuilder<T>& program,
    std::vector<std::unique_ptr<OperatorTranslator<T>>> children)
    : OperatorTranslator<T>(std::move(children)),
      order_by_(order_by),
      program_(program),
      expr_translator_(program, *this) {}

template <typename T>
void OrderByTranslator<T>::Produce() {
  // include every child column inside the struct
  proxy::StructBuilder<T> packed(program_);
  const auto& child_schema = order_by_.Child().Schema().Columns();
  for (const auto& col : child_schema) {
    switch (col.Expr().Type()) {
      case catalog::SqlType::SMALLINT:
        packed.Add(proxy::Int16<T>::Type);
        break;
      case catalog::SqlType::INT:
        packed.Add(proxy::Int32<T>::Type);
        break;
      case catalog::SqlType::BIGINT:
        packed.Add(proxy::Int64<T>::Type);
        break;
      case catalog::SqlType::REAL:
        packed.Add(proxy::Float64<T>::Type);
        break;
      case catalog::SqlType::DATE:
        packed.Add(proxy::Int64<T>::Type);
        break;
      case catalog::SqlType::TEXT:
        throw std::runtime_error("Text not supported at the moment.");
        break;
      case catalog::SqlType::BOOLEAN:
        packed.Add(proxy::Bool<T>::Type);
        break;
    }
  }

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
  buffer_ = std::make_unique<proxy::Vector<T>>(program_, packed);

  // populate vector
  this->Child().Produce();

  // sort
  buffer_->Sort();

  proxy::IndexLoop<T>(
      program_, [&]() { return proxy::UInt32<T>(program_, 0); },
      [&](proxy::UInt32<T>& i) { return i < buffer_->Size(); },
      [&](proxy::UInt32<T>& i) {
        auto values = buffer_->Get(i).Unpack();

        // set the child variables
        this->Child().SchemaValues().Values();

        for (const auto& column : order_by_.Schema().Columns()) {
          this->values_.AddVariable(expr_translator_.Compute(column.Expr()));
        }

        if (auto parent = this->Parent()) {
          parent->get().Consume(*this);
        }

        return i + proxy::UInt32<T>(program_, 1);
      });

  buffer_.reset();
}

template <typename T>
void OrderByTranslator<T>::Consume(OperatorTranslator<T>& src) {
  buffer_->Append().Pack(this->Child().SchemaValues().Values());
}

INSTANTIATE_ON_IR(OrderByTranslator);

}  // namespace kush::compile