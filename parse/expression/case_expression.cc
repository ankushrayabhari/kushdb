#include "parse/expression/case_expression.h"

#include <memory>
#include <vector>

#include "parse/expression/expression.h"

namespace kush::parse {

CaseExpression::CaseExpression(
    std::vector<
        std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>>
        cases,
    std::unique_ptr<Expression> fallthrough)
    : cases_(std::move(cases)), fallthrough_(std::move(fallthrough)) {}

}  // namespace kush::parse