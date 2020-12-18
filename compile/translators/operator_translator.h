#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "compile/schema_values.h"

namespace kush::compile {

class OperatorTranslator {
 public:
  OperatorTranslator(std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~OperatorTranslator() = default;

  virtual void Produce() = 0;
  virtual void Consume(OperatorTranslator& src) = 0;
  std::optional<std::reference_wrapper<OperatorTranslator>> Parent();
  std::vector<std::reference_wrapper<OperatorTranslator>> Children();
  OperatorTranslator& Child();
  OperatorTranslator& LeftChild();
  OperatorTranslator& RightChild();
  SchemaValues& GetValues();
  SchemaValues& GetVirtualValues();

 protected:
  SchemaValues values_;
  SchemaValues virtual_values_;

 private:
  OperatorTranslator* parent_;
  std::vector<std::unique_ptr<OperatorTranslator>> children_;
};

}  // namespace kush::compile