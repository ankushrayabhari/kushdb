#include "compile/translators/operator_translator.h"

#include <memory>
#include <optional>
#include <vector>

#include "compile/ir_registry.h"
#include "compile/translators/schema_values.h"
#include "util/vector_util.h"

namespace kush::compile {

template <typename T>
OperatorTranslator<T>::OperatorTranslator(
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : parent_(nullptr), children_(std::move(children)) {
  for (auto& child : children_) {
    child->parent_ = this;
  }
}

template <typename T>
std::optional<std::reference_wrapper<OperatorTranslator<T>>>
OperatorTranslator<T>::Parent() {
  if (parent_ == nullptr) {
    return std::nullopt;
  }

  return *parent_;
}

template <typename T>
std::vector<std::reference_wrapper<OperatorTranslator<T>>>
OperatorTranslator<T>::Children() {
  return util::ReferenceVector(children_);
}

template <typename T>
OperatorTranslator<T>& OperatorTranslator<T>::Child() {
  return *children_[0];
}

template <typename T>
OperatorTranslator<T>& OperatorTranslator<T>::LeftChild() {
  return *children_[0];
}

template <typename T>
OperatorTranslator<T>& OperatorTranslator<T>::RightChild() {
  return *children_[1];
}

template <typename T>
SchemaValues<T>& OperatorTranslator<T>::SchemaValues() {
  return values_;
}

template <typename T>
const SchemaValues<T>& OperatorTranslator<T>::SchemaValues() const {
  return values_;
}

template <typename T>
const SchemaValues<T>& OperatorTranslator<T>::VirtualSchemaValues() const {
  return virtual_values_;
}

INSTANTIATE_ON_IR(OperatorTranslator);

}  // namespace kush::compile