#include "compile/translators/operator_translator.h"

#include <memory>
#include <optional>
#include <vector>

#include "compile/translators/schema_values.h"
#include "util/vector_util.h"

namespace kush::compile {

OperatorTranslator::OperatorTranslator(
    const plan::Operator& op,
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

SchemaValues& OperatorTranslator::SchemaValues() { return values_; }

const SchemaValues& OperatorTranslator::SchemaValues() const { return values_; }

const SchemaValues& OperatorTranslator::VirtualSchemaValues() const {
  return virtual_values_;
}

}  // namespace kush::compile