#pragma once

#include <string>
#include <utility>
#include <vector>

#include "compile/khir/khir_program_builder.h"
#include "compile/proxy/vector.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "plan/order_by_operator.h"

namespace kush::compile {

class OrderByTranslator : public OperatorTranslator {
 public:
  OrderByTranslator(const plan::OrderByOperator& order_by,
                    khir::KHIRProgramBuilder& program,
                    std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~OrderByTranslator() = default;

  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  const plan::OrderByOperator& order_by_;
  khir::KHIRProgramBuilder& program_;
  ExpressionTranslator expr_translator_;
  std::unique_ptr<proxy::Vector> buffer_;
};

}  // namespace kush::compile