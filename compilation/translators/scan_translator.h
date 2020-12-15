#pragma once

#include "compilation/cpp_translator.h"
#include "compilation/translators/translator.h"
#include "plan/operator.h"

namespace kush::compile {

class ScanTranslator : public Translator {
 public:
  ScanTranslator(plan::Scan& scan, CppTranslator& context,
                 std::vector<std::unique_ptr<Translator>> children);
  void Produce() override;
  void Consume(Translator& src) override;

 private:
  plan::Scan& scan_;
  CppTranslator& context_;
};

}  // namespace kush::compile