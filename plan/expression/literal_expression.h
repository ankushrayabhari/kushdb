#pragma once

#include <cstdint>
#include <functional>
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
  explicit LiteralExpression(bool value);
  explicit LiteralExpression(catalog::SqlType type);
  ~LiteralExpression() = default;

  void Visit(std::function<void(int16_t, bool)>,
             std::function<void(int32_t, bool)>,
             std::function<void(int64_t, bool)>,
             std::function<void(double, bool)>,
             std::function<void(std::string, bool)>,
             std::function<void(bool, bool)>,
             std::function<void(absl::CivilDay, bool)>) const;
  void Accept(ExpressionVisitor& visitor) override;
  void Accept(ImmutableExpressionVisitor& visitor) const override;

  nlohmann::json ToJson() const override;

 private:
  std::variant<int16_t, int32_t, int64_t, double, std::string, bool,
               absl::CivilDay>
      value_;
  bool null_;
};

}  // namespace kush::plan