#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "compile/translators/schema_values.h"
#include "plan/operator/operator.h"

namespace kush::compile {

class OperatorTranslator {
 public:
  OperatorTranslator(const plan::Operator& op,
                     std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~OperatorTranslator() = default;

  virtual void Produce() = 0;
  virtual void Consume(OperatorTranslator& src) = 0;

  std::optional<std::reference_wrapper<OperatorTranslator>> Parent();
  std::vector<std::reference_wrapper<OperatorTranslator>> Children();
  OperatorTranslator& Child();
  OperatorTranslator& LeftChild();
  OperatorTranslator& RightChild();
  kush::compile::SchemaValues& SchemaValues();
  const kush::compile::SchemaValues& SchemaValues() const;
  const kush::compile::SchemaValues& VirtualSchemaValues() const;

 protected:
  kush::compile::SchemaValues values_;
  kush::compile::SchemaValues virtual_values_;

 private:
  OperatorTranslator* parent_;
  std::vector<std::unique_ptr<OperatorTranslator>> children_;
};

}  // namespace kush::compile