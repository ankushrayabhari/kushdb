#include "compile/translators/skinner_join_translator.h"

#include <string>
#include <utility>
#include <vector>

#include "compile/ir_registry.h"
#include "compile/proxy/printer.h"
#include "compile/proxy/struct.h"
#include "compile/proxy/value.h"
#include "compile/proxy/vector.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "compile/translators/scan_translator.h"
#include "util/vector_util.h"

namespace kush::compile {

template <typename T>
SkinnerJoinTranslator<T>::SkinnerJoinTranslator(
    const plan::SkinnerJoinOperator& join, ProgramBuilder<T>& program,
    std::vector<std::unique_ptr<OperatorTranslator<T>>> children)
    : OperatorTranslator<T>(std::move(children)),
      join_(join),
      program_(program),
      expr_translator_(program_, *this) {}

template <typename T>
void SkinnerJoinTranslator<T>::Produce() {
  // 1. Materialize each child.
  auto child_translators = this->Children();
  auto child_operators = this->join_.Children();
  std::vector<std::unique_ptr<proxy::StructBuilder<T>>> structs;
  for (int i = 0; i < child_translators.size(); i++) {
    auto& child_translator = child_translators[i].get();
    auto& child_operator = child_operators[i].get();

    if (auto scan = dynamic_cast<ScanTranslator<T>*>(&child_translator)) {
    } else {
      // Create struct for materialization
      structs.push_back(std::make_unique<proxy::StructBuilder<T>>(program_));
      const auto& child_schema = child_operator.Schema().Columns();
      for (const auto& col : child_schema) {
        structs.back()->Add(col.Expr().Type());
      }
      structs.back()->Build();

      // Init vector of structs
      buffers_.push_back(
          std::make_unique<proxy::Vector<T>>(program_, *structs.back()));

      // Fill buffer
      child_translator.Produce();
    }
  }

  proxy::Printer printer(program_);
  for (auto& buffer : buffers_) {
    if (buffer != nullptr) {
      buffer->Size().Print(printer);
    }
  }
  printer.PrintNewline();

  // TODO:
  // 2. Setup join evaluation
  // 3. Execute join

  // Cleanup
  for (auto& buffer : buffers_) {
    if (buffer != nullptr) {
      buffer.reset();
    }
  }
}

template <typename T>
void SkinnerJoinTranslator<T>::Consume(OperatorTranslator<T>& src) {
  // Last element of buffers is the materialization for src operator.
  buffers_.back()->PushBack().Pack(src.SchemaValues().Values());
}

INSTANTIATE_ON_IR(SkinnerJoinTranslator);

}  // namespace kush::compile