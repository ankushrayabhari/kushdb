#pragma once

#include "compilation/translators/translator.h"
#include "plan/operator.h"

namespace kush::compile {

class OutputTranslator : public Translator<plan::Output> {
 public:
  void Produce(plan::Output& op) override;
  void Consume(plan::Output& op, plan::Operator& src) override;
};

}  // namespace kush::compile