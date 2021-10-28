#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

#include "absl/time/civil_time.h"

#include "parse/expression/expression.h"

namespace kush::parse {

class LiteralExpression : public Expression {
 public:
  explicit LiteralExpression(int16_t value);
  explicit LiteralExpression(int32_t value);
  explicit LiteralExpression(int64_t value);
  explicit LiteralExpression(double value);
  explicit LiteralExpression(absl::CivilDay value);
  explicit LiteralExpression(std::string_view value);
  explicit LiteralExpression(bool value);
  ~LiteralExpression() = default;

  void Visit(std::function<void(int16_t)>, std::function<void(int32_t)>,
             std::function<void(int64_t)>, std::function<void(double)>,
             std::function<void(std::string)>, std::function<void(bool)>,
             std::function<void(absl::CivilDay)>) const;

  std::string GetValue() const;

 private:
  std::variant<int16_t, int32_t, int64_t, double, std::string, bool,
               absl::CivilDay>
      value_;
};

}  // namespace kush::parse