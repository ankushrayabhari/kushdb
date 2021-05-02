#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

#include "absl/time/civil_time.h"

#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

class LiteralExpression : public Expression {
 public:
  explicit LiteralExpression(int16_t value);
  explicit LiteralExpression(int32_t value);
  explicit LiteralExpression(int64_t value);
  explicit LiteralExpression(double value);
  explicit LiteralExpression(absl::CivilDay value);
  explicit LiteralExpression(std::string_view value);
  explicit LiteralExpression(const char* value);
  explicit LiteralExpression(bool value);
  ~LiteralExpression() = default;

  int16_t GetSmallintValue() const;
  int32_t GetIntValue() const;
  int64_t GetBigintValue() const;
  double GetRealValue() const;
  int64_t GetDateValue() const;
  std::string_view GetTextValue() const;
  bool GetBooleanValue() const;

  void Accept(ExpressionVisitor& visitor) override;
  void Accept(ImmutableExpressionVisitor& visitor) const override;

  nlohmann::json ToJson() const override;

 private:
  std::variant<int16_t, int32_t, int64_t, double, std::string, bool> value_;
};

}  // namespace kush::plan