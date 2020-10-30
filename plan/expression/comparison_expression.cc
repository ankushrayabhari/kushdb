#include "plan/expression/comparison_expression.h"

#include <memory>

#include "magic_enum.hpp"
#include "nlohmann/json.hpp"
#include "plan/expression/expression.h"

namespace kush::plan {

ComparisonExpression::ComparisonExpression(ComparisonType type,
                                           std::unique_ptr<Expression> left,
                                           std::unique_ptr<Expression> right)
    : type_(type), left_(std::move(left)), right_(std::move(right)) {}

nlohmann::json ComparisonExpression::ToJson() const {
  nlohmann::json j;
  j["op"] = magic_enum::enum_name(type_);
  j["left"] = left_->ToJson();
  j["right"] = right_->ToJson();
  return j;
}

}  // namespace kush::plan