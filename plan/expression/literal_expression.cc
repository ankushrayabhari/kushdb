#include "plan/expression/literal_expression.h"

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

#include "absl/time/civil_time.h"
#include "absl/time/time.h"

#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"

namespace kush::plan {

template <class>
inline constexpr bool always_false_v = false;

LiteralExpression::LiteralExpression(int16_t value)
    : Expression(catalog::SqlType::SMALLINT, false, {}),
      value_(value),
      null_(false) {}

LiteralExpression::LiteralExpression(int32_t value)
    : Expression(catalog::SqlType::INT, false, {}),
      value_(value),
      null_(false) {}

LiteralExpression::LiteralExpression(int64_t value)
    : Expression(catalog::SqlType::BIGINT, false, {}),
      value_(value),
      null_(false) {}

LiteralExpression::LiteralExpression(double value)
    : Expression(catalog::SqlType::REAL, false, {}),
      value_(value),
      null_(false) {}

LiteralExpression::LiteralExpression(absl::CivilDay value)
    : Expression(catalog::SqlType::DATE, false, {}),
      value_(value),
      null_(false) {}

LiteralExpression::LiteralExpression(std::string_view value)
    : Expression(catalog::SqlType::TEXT, false, {}),
      value_(std::string(value)),
      null_(false) {}

LiteralExpression::LiteralExpression(bool value)
    : Expression(catalog::SqlType::BOOLEAN, false, {}),
      value_(value),
      null_(false) {}

LiteralExpression::LiteralExpression(catalog::SqlType type)
    : Expression(type, true, {}), null_(true) {
  switch (type) {
    case catalog::SqlType::BOOLEAN:
      value_ = false;
      break;

    case catalog::SqlType::SMALLINT:
      value_ = (int16_t)0;
      break;

    case catalog::SqlType::INT:
      value_ = (int32_t)0;
      break;

    case catalog::SqlType::BIGINT:
      value_ = (int64_t)0;
      break;

    case catalog::SqlType::REAL:
      value_ = 0.0;
      break;

    case catalog::SqlType::TEXT:
      value_ = std::string();
      break;

    case catalog::SqlType::DATE:
      value_ = absl::CivilDay(2000, 0, 0);
      break;
  }
}

void LiteralExpression::Visit(
    std::function<void(int16_t, bool)> f1,
    std::function<void(int32_t, bool)> f2,
    std::function<void(int64_t, bool)> f3, std::function<void(double, bool)> f4,
    std::function<void(std::string, bool)> f5,
    std::function<void(bool, bool)> f6,
    std::function<void(absl::CivilDay, bool)> f7) const {
  std::visit(
      [&](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, int16_t>) {
          f1(arg, null_);
        } else if constexpr (std::is_same_v<T, int32_t>) {
          f2(arg, null_);
        } else if constexpr (std::is_same_v<T, int64_t>) {
          f3(arg, null_);
        } else if constexpr (std::is_same_v<T, double>) {
          f4(arg, null_);
        } else if constexpr (std::is_same_v<T, std::string>) {
          f5(arg, null_);
        } else if constexpr (std::is_same_v<T, bool>) {
          f6(arg, null_);
        } else if constexpr (std::is_same_v<T, absl::CivilDay>) {
          f7(arg, null_);
        } else {
          static_assert(always_false_v<T>, "unreachable");
        }
      },
      value_);
}

nlohmann::json LiteralExpression::ToJson() const {
  nlohmann::json res;
  Visit(
      [&](int16_t v, bool null) {
        res["value"] = v;
        res["null"] = null;
      },
      [&](int32_t v, bool null) {
        res["value"] = v;
        res["null"] = null;
      },
      [&](int64_t v, bool null) {
        res["value"] = v;
        res["null"] = null;
      },
      [&](double v, bool null) {
        res["value"] = v;
        res["null"] = null;
      },
      [&](std::string v, bool null) {
        res["value"] = v;
        res["null"] = null;
      },
      [&](bool v, bool null) {
        res["value"] = v;
        res["null"] = null;
      },
      [&](absl::CivilDay v, bool null) {
        res["value"] = absl::FormatCivilTime(v);
        res["null"] = null;
      });
  return res;
}

void LiteralExpression::Accept(ExpressionVisitor& visitor) {
  return visitor.Visit(*this);
}

void LiteralExpression::Accept(ImmutableExpressionVisitor& visitor) const {
  return visitor.Visit(*this);
}

}  // namespace kush::plan