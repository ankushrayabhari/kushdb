#pragma once

#include <string>
#include <utility>
#include <vector>

#include "compile/cpp/cpp_compilation_context.h"
#include "compile/cpp/translators/expression_translator.h"
#include "compile/cpp/translators/operator_translator.h"
#include "plan/order_by_operator.h"

namespace kush::compile::cpp {

class OrderByTranslator : public OperatorTranslator {
 public:
  OrderByTranslator(const plan::OrderByOperator& order_by,
                    CppCompilationContext& context,
                    std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~OrderByTranslator() = default;

  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  const plan::OrderByOperator& order_by_;
  CppCompilationContext& context_;
  ExpressionTranslator expr_translator_;
  std::string packed_struct_id_;
  std::vector<std::pair<std::string, std::string>> packed_field_type_;
  std::string buffer_var_;
};

}  // namespace kush::compile::cpp