#pragma once

#include <memory>
#include <vector>

#include "compilation/schema_values.h"

namespace kush::compile {

class OperatorTranslator {
 public:
  OperatorTranslator(std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~OperatorTranslator() = default;

  virtual void Produce() = 0;
  virtual void Consume(OperatorTranslator& src) = 0;
  void SetParent(OperatorTranslator& parent);
  std::vector<std::reference_wrapper<OperatorTranslator>> Children();
  SchemaValues& GetValues();

 private:
  OperatorTranslator* parent_;
  std::vector<std::unique_ptr<OperatorTranslator>> children_;
  std::unique_ptr<SchemaValues> values_;
};

}  // namespace kush::compile