#pragma once

#include "compilation/translators/translator.h"
#include "plan/operator.h"

namespace kush::compile {

class HashJoinTranslator : public Translator<plan::HashJoin> {
 public:
  void Produce(plan::HashJoin& op) override;
  void Consume(plan::HashJoin& op, plan::Operator& src) override;
};

}  // namespace kush::compile