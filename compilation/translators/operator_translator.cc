#include "compilation/translators/operator_translator.h"

namespace kush::compile {

OperatorTranslator::OperatorTranslator(
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : parent_(nullptr), children_(std::move(children)) {
  for (auto& child : children_) {
    child->SetParent(*this);
  }
}

void OperatorTranslator::SetParent(OperatorTranslator& translator) {
  parent_ = &translator;
}

std::vector<std::reference_wrapper<OperatorTranslator>>
OperatorTranslator::Children() {
  std::vector<std::reference_wrapper<OperatorTranslator>> output;
  for (auto& x : children_) {
    output.push_back(*x);
  }
  return output;
}

SchemaValues& OperatorTranslator::GetValues() { return *values_; }

}  // namespace kush::compile