#pragma once

#include "compile/proxy/printer.h"
#include "compile/translators/operator_translator.h"
#include "plan/output_operator.h"

namespace kush::compile {

template <typename T>
class OutputTranslator : public OperatorTranslator<T> {
 public:
  OutputTranslator(
      const plan::OutputOperator& output, ProgramBuilder<T>& program,
      std::vector<std::unique_ptr<OperatorTranslator<T>>> children);
  virtual ~OutputTranslator() = default;

  void Produce() override;
  void Consume(OperatorTranslator<T>& src) override;

 private:
  ProgramBuilder<T>& program_;
};

}  // namespace kush::compile