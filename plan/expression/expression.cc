#include "plan/expression/expression.h"

#include <cstdint>

#include "catalog/sql_type.h"
#include "nlohmann/json.hpp"
#include "plan/expression/expression_visitor.h"
#include "util/vector_util.h"

namespace kush::plan {

Expression::Expression(catalog::SqlType type,
                       std::vector<std::unique_ptr<Expression>> children)
    : type_(type), children_(std::move(children)) {}

std::vector<std::reference_wrapper<Expression>> Expression::Children() {
  return util::ReferenceVector(children_);
}

catalog::SqlType Expression::Type() const { return type_; }

UnaryExpression::UnaryExpression(catalog::SqlType type,
                                 std::unique_ptr<Expression> child)
    : Expression(type, util::MakeVector(std::move(child))) {}

Expression& UnaryExpression::Child() { return *children_[0]; }

const Expression& UnaryExpression::Child() const { return *children_[0]; }

BinaryExpression::BinaryExpression(catalog::SqlType type,
                                   std::unique_ptr<Expression> left_child,
                                   std::unique_ptr<Expression> right_child)
    : Expression(type, util::MakeVector(std::move(left_child),
                                        std::move(right_child))) {}

Expression& BinaryExpression::LeftChild() { return *children_[0]; }

const Expression& BinaryExpression::LeftChild() const { return *children_[0]; }

Expression& BinaryExpression::RightChild() { return *children_[1]; }

const Expression& BinaryExpression::RightChild() const { return *children_[1]; }

}  // namespace kush::plan