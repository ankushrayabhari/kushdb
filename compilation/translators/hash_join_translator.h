#pragma once

#include "compilation/cpp_translator.h"
#include "compilation/translators/operator_translator.h"
#include "plan/operator.h"

namespace kush::compile {

class HashJoinTranslator : public OperatorTranslator {
 public:
  HashJoinTranslator(plan::HashJoin& output, CppTranslator& context,
                     std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~HashJoinTranslator() = default;
  void Produce() override;
  void Consume(OperatorTranslator& src) override;
};

}  // namespace kush::compile