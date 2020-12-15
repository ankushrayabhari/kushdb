#pragma once

#include <memory>
#include <vector>

#include "compilation/cpp_translator.h"
#include "plan/expression/expression_visitor.h"

namespace kush::compile {

class ExpressionTranslator : public plan::ExpressionVisitor {
 public:
  ExpressionTranslator(CppTranslator& context);
  virtual ~ExpressionTranslator() = default;

  void Visit(plan::ColumnRefExpression& scan) override;
  void Visit(plan::ComparisonExpression& select) override;
  void Visit(plan::LiteralExpression<int32_t>& output) override;

 private:
  CppTranslator& context_;
};

}  // namespace kush::compile