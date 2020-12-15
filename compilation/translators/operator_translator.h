#pragma once

#include <memory>
#include <vector>

namespace kush::compile {

class OperatorTranslator {
 public:
  OperatorTranslator(std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~OperatorTranslator() = default;

  virtual void Produce() = 0;
  virtual void Consume(OperatorTranslator& src) = 0;
  void SetParent(OperatorTranslator& parent);

 private:
  OperatorTranslator* parent_;
  std::vector<std::unique_ptr<OperatorTranslator>> children_;
};

}  // namespace kush::compile