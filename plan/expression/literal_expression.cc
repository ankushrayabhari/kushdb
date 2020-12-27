#include "plan/expression/literal_expression.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

#include "absl/time/civil_time.h"
#include "absl/time/time.h"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

LiteralExpression::LiteralExpression(int16_t value)
    : Expression(catalog::SqlType::SMALLINT, {}), value_(value) {}

LiteralExpression::LiteralExpression(int32_t value)
    : Expression(catalog::SqlType::INT, {}), value_(value) {}

LiteralExpression::LiteralExpression(int64_t value)
    : Expression(catalog::SqlType::BIGINT, {}), value_(value) {}

LiteralExpression::LiteralExpression(double value)
    : Expression(catalog::SqlType::REAL, {}), value_(value) {}

LiteralExpression::LiteralExpression(absl::CivilDay value)
    : Expression(catalog::SqlType::DATE, {}) {
  // set value to converted timestamp
  absl::TimeZone utc = absl::UTCTimeZone();
  absl::Time time = absl::FromCivil(value, utc);
  value_ = absl::ToUnixMillis(time);
}

LiteralExpression::LiteralExpression(const char* value)
    : Expression(catalog::SqlType::TEXT, {}), value_(std::string(value)) {}

LiteralExpression::LiteralExpression(std::string_view value)
    : Expression(catalog::SqlType::TEXT, {}), value_(std::string(value)) {}

LiteralExpression::LiteralExpression(bool value)
    : Expression(catalog::SqlType::BOOLEAN, {}), value_(value) {}

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

int64_t LiteralExpression::GetDateValue() const {
  return std::get<int64_t>(value_);
}

std::string_view LiteralExpression::GetTextValue() const {
  return std::get<std::string>(value_);
}

bool LiteralExpression::GetBooleanValue() const {
  return std::get<bool>(value_);
}

nlohmann::json LiteralExpression::ToJson() const {
  nlohmann::json j;
  switch (Type()) {
    case catalog::SqlType::SMALLINT:
      j["value"] = GetSmallintValue();
      break;
    case catalog::SqlType::INT:
      j["value"] = GetIntValue();
      break;
    case catalog::SqlType::BIGINT:
      j["value"] = GetBigintValue();
      break;
    case catalog::SqlType::DATE:
      j["value"] = GetDateValue();
      break;
    case catalog::SqlType::REAL:
      j["value"] = GetRealValue();
      break;
    case catalog::SqlType::TEXT:
      j["value"] = GetTextValue();
      break;
    case catalog::SqlType::BOOLEAN:
      j["value"] = GetBooleanValue();
      break;
  }
  return j;
}

void LiteralExpression::Accept(ExpressionVisitor& visitor) {
  return visitor.Visit(*this);
}

void LiteralExpression::Accept(ImmutableExpressionVisitor& visitor) const {
  return visitor.Visit(*this);
}

}  // namespace kush::plan