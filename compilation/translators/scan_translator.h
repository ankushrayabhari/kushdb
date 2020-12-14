#pragma once

#include "compilation/translators/translator.h"
#include "plan/operator.h"

namespace kush::compile {

class ScanTranslator : public Translator<plan::Scan> {
 public:
  void Produce(plan::Scan& op) override;
  void Consume(plan::Scan& op, plan::Operator& src) override;
};

}  // namespace kush::compile