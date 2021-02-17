#pragma once

#include <string>
#include <utility>
#include <vector>

#include "compile/proxy/vector.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "plan/order_by_operator.h"

namespace kush::compile {

template <typename T>
class OrderByTranslator : public OperatorTranslator<T> {
 public:
  OrderByTranslator(
      const plan::OrderByOperator& order_by, ProgramBuilder<T>& program,
      proxy::ForwardDeclaredVectorFunctions<T>& vector_funcs,
      std::vector<std::unique_ptr<OperatorTranslator<T>>> children);
  virtual ~OrderByTranslator() = default;

  void Produce() override;
  void Consume(OperatorTranslator<T>& src) override;

 private:
  const plan::OrderByOperator& order_by_;
  ProgramBuilder<T>& program_;
  ExpressionTranslator<T> expr_translator_;
  proxy::ForwardDeclaredVectorFunctions<T>& vector_funcs_;
  std::unique_ptr<proxy::Vector<T>> buffer_;
};

}  // namespace kush::compile