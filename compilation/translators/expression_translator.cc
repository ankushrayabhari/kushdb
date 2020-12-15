#include "compilation/translators/expression_translator.h"

#include <memory>
#include <vector>

#include "compilation/cpp_translator.h"
#include "plan/expression/expression_visitor.h"

namespace kush::compile {

ExpressionTranslator::ExpressionTranslator(CppTranslator& context)
    : context_(context) {}

void ExpressionTranslator::Visit(plan::ColumnRefExpression& scan) {}

void ExpressionTranslator::Visit(plan::ComparisonExpression& select) {}

void ExpressionTranslator::Visit(plan::LiteralExpression<int32_t>& output) {}

}  // namespace kush::compile