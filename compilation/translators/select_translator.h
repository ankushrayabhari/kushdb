#pragma once

#include "compilation/translators/translator.h"
#include "plan/operator.h"

namespace kush::compile {

class SelectTranslator : public Translator<plan::Select> {
 public:
  void Produce(plan::Select& op) override;
  void Consume(plan::Select& op, plan::Operator& src) override;
};

}  // namespace kush::compile