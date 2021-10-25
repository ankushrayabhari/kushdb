#include "parse/expression/literal_expression.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

#include "absl/time/civil_time.h"
#include "absl/time/time.h"

#include "parse/expression/expression.h"

namespace kush::parse {

LiteralExpression::LiteralExpression(int16_t value) : value_(value) {}

LiteralExpression::LiteralExpression(int32_t value) : value_(value) {}

LiteralExpression::LiteralExpression(int64_t value) : value_(value) {}

LiteralExpression::LiteralExpression(double value) : value_(value) {}

LiteralExpression::LiteralExpression(absl::CivilDay value) : value_(value) {}

LiteralExpression::LiteralExpression(std::string_view value)
    : value_(std::string(value)) {}

LiteralExpression::LiteralExpression(std::nullptr_t value) : value_(value) {}

LiteralExpression::LiteralExpression(bool value) : value_(value) {}

int16_t LiteralExpression::GetSmallintValue() const {
  return std::get<int16_t>(value_);
}

int32_t LiteralExpression::GetIntValue() const {
  return std::get<int32_t>(value_);
}

int64_t LiteralExpression::GetBigintValue() const {
  return std::get<int64_t>(value_);
}

double LiteralExpression::GetRealValue() const {
  return std::get<double>(value_);
}

absl::CivilDay LiteralExpression::GetDateValue() const {
  return std::get<absl::CivilDay>(value_);
}

std::string_view LiteralExpression::GetTextValue() const {
  return std::get<std::string>(value_);
}

bool LiteralExpression::GetBooleanValue() const {
  return std::get<bool>(value_);
}

bool LiteralExpression::IsNull() const {
  return std::holds_alternative<std::nullptr_t>(value_);
}

}  // namespace kush::parse