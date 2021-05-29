#pragma once

#include "compile/khir/program_builder.h"
#include "compile/proxy/printer.h"
#include "compile/translators/operator_translator.h"
#include "plan/output_operator.h"

namespace kush::compile {

class OutputTranslator : public OperatorTranslator {
 public:
  OutputTranslator(const plan::OutputOperator& output,
                   khir::ProgramBuilder& program,
                   std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~OutputTranslator() = default;

  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  khir::ProgramBuilder& program_;
};

}  // namespace kush::compile