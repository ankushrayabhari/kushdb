#include "compile/translators/operator_translator.h"

#include "util/vector_util.h"

namespace kush::compile {

OperatorTranslator::OperatorTranslator(
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : parent_(nullptr), children_(std::move(children)) {
  for (auto& child : children_) {
    child->parent_ = this;
  }
}

std::optional<std::reference_wrapper<OperatorTranslator>>
OperatorTranslator::Parent() {
  if (parent_ == nullptr) {
    return std::nullopt;
  }

  return *parent_;
}

std::vector<std::reference_wrapper<OperatorTranslator>>
OperatorTranslator::Children() {
  return util::ReferenceVector(children_);
}

OperatorTranslator& OperatorTranslator::Child() { return *children_[0]; }

OperatorTranslator& OperatorTranslator::LeftChild() { return *children_[0]; }

OperatorTranslator& OperatorTranslator::RightChild() { return *children_[1]; }

const SchemaValues& OperatorTranslator::GetValues() const { return values_; }

const SchemaValues& OperatorTranslator::GetVirtualValues() const {
  return virtual_values_;
}

}  // namespace kush::compile