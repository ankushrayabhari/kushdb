#pragma once

#include <string>
#include <utility>
#include <vector>

#include "compile/proxy/vector.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "khir/program_builder.h"
#include "plan/order_by_operator.h"

namespace kush::compile {

class OrderByTranslator : public OperatorTranslator {
 public:
  OrderByTranslator(const plan::OrderByOperator& order_by,
                    khir::ProgramBuilder& program,
                    std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~OrderByTranslator() = default;

  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  const plan::OrderByOperator& order_by_;
  khir::ProgramBuilder& program_;
  ExpressionTranslator expr_translator_;
  std::unique_ptr<proxy::Vector> buffer_;
};

}  // namespace kush::compile