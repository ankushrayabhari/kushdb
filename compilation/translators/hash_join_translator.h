#pragma once

#include "compilation/cpp_translator.h"
#include "compilation/translators/translator.h"
#include "plan/operator.h"

namespace kush::compile {

class HashJoinTranslator : public Translator {
 public:
  HashJoinTranslator(plan::HashJoin& output, CppTranslator& context,
                     std::vector<std::unique_ptr<Translator>> children);
  void Produce() override;
  void Consume(Translator& src) override;
};

}  // namespace kush::compile