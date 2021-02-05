#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "compile/translators/schema_values.h"

namespace kush::compile {

template <typename T>
class OperatorTranslator {
 public:
  OperatorTranslator(
      std::vector<std::unique_ptr<OperatorTranslator<T>>> children);
  virtual ~OperatorTranslator() = default;

  virtual void Produce() = 0;
  virtual void Consume(OperatorTranslator<T>& src) = 0;

  std::optional<std::reference_wrapper<OperatorTranslator<T>>> Parent();
  std::vector<std::reference_wrapper<OperatorTranslator<T>>> Children();
  OperatorTranslator<T>& Child();
  OperatorTranslator<T>& LeftChild();
  OperatorTranslator<T>& RightChild();
  const kush::compile::SchemaValues<T>& SchemaValues() const;
  const kush::compile::SchemaValues<T>& VirtualSchemaValues() const;

 protected:
  kush::compile::SchemaValues<T> values_;
  kush::compile::SchemaValues<T> virtual_values_;

 private:
  OperatorTranslator<T>* parent_;
  std::vector<std::unique_ptr<OperatorTranslator<T>>> children_;
};

}  // namespace kush::compile