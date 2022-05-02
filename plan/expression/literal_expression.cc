#include "plan/expression/literal_expression.h"

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

#include "magic_enum.hpp"

#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"
#include "runtime/date.h"
#include "runtime/enum.h"

namespace kush::plan {

template <class>
inline constexpr bool always_false_v = false;

LiteralExpression::LiteralExpression(int16_t value)
    : Expression(catalog::Type::SmallInt(), false, {}),
      value_(value),
      null_(false) {}

LiteralExpression::LiteralExpression(int32_t value)
    : Expression(catalog::Type::Int(), false, {}),
      value_(value),
      null_(false) {}

LiteralExpression::LiteralExpression(int64_t value)
    : Expression(catalog::Type::BigInt(), false, {}),
      value_(value),
      null_(false) {}

LiteralExpression::LiteralExpression(double value)
    : Expression(catalog::Type::Real(), false, {}),
      value_(value),
      null_(false) {}

LiteralExpression::LiteralExpression(runtime::Date::DateBuilder value)
    : Expression(catalog::Type::Date(), false, {}),
      value_(value),
      null_(false) {}

LiteralExpression::LiteralExpression(std::string_view value)
    : Expression(catalog::Type::Text(), false, {}),
      value_(std::string(value)),
      null_(false) {}

LiteralExpression::LiteralExpression(bool value)
    : Expression(catalog::Type::Boolean(), false, {}),
      value_(value),
      null_(false) {}

LiteralExpression::LiteralExpression(int32_t enum_id, std::string_view value)
    : Expression(catalog::Type::Enum(enum_id), false, {}),
      value_(EnumValue{.value = runtime::Enum::EnumManager::Get().GetValue(
                           enum_id, std::string(value))}),
      null_(false) {}

LiteralExpression::LiteralExpression(const catalog::Type& type)
    : Expression(type, true, {}), null_(true) {
  switch (type.type_id) {
    case catalog::TypeId::BOOLEAN:
      value_ = false;
      break;

    case catalog::TypeId::SMALLINT:
      value_ = (int16_t)0;
      break;

    case catalog::TypeId::INT:
      value_ = (int32_t)0;
      break;

    case catalog::TypeId::BIGINT:
      value_ = (int64_t)0;
      break;

    case catalog::TypeId::REAL:
      value_ = 0.0;
      break;

    case catalog::TypeId::TEXT:
      value_ = std::string();
      break;

    case catalog::TypeId::DATE:
      value_ = runtime::Date::DateBuilder(2000, 1, 1);
      break;

    case catalog::TypeId::ENUM:
      value_ = EnumValue{.value = -1};
      break;
  }
}

void LiteralExpression::Visit(
    std::function<void(int16_t, bool)> f1,
    std::function<void(int32_t, bool)> f2,
    std::function<void(int64_t, bool)> f3, std::function<void(double, bool)> f4,
    std::function<void(std::string, bool)> f5,
    std::function<void(bool, bool)> f6,
    std::function<void(runtime::Date::DateBuilder, bool)> f7,
    std::function<void(int32_t, int32_t, bool)> f8) const {
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
        } else if constexpr (std::is_same_v<T, runtime::Date::DateBuilder>) {
          f7(arg, null_);
        } else if constexpr (std::is_same_v<T, EnumValue>) {
          EnumValue v = arg;
          f8(v.value, this->Type().enum_id, null_);
        } else {
          static_assert(always_false_v<T>, "unreachable");
        }
      },
      value_);
}

nlohmann::json LiteralExpression::ToJson() const {
  nlohmann::json j;
  j["type"] = this->Type().ToString();
  Visit(
      [&](int16_t v, bool null) {
        j["value"] = v;
        j["null"] = null;
      },
      [&](int32_t v, bool null) {
        j["value"] = v;
        j["null"] = null;
      },
      [&](int64_t v, bool null) {
        j["value"] = v;
        j["null"] = null;
      },
      [&](double v, bool null) {
        j["value"] = v;
        j["null"] = null;
      },
      [&](std::string v, bool null) {
        j["value"] = v;
        j["null"] = null;
      },
      [&](bool v, bool null) {
        j["value"] = v;
        j["null"] = null;
      },
      [&](runtime::Date::DateBuilder v, bool null) {
        j["value"] = std::to_string(v.year) + "-" + std::to_string(v.month) +
                     "-" + std::to_string(v.day);
        j["null"] = null;
      },
      [&](auto idx, auto enum_id, bool null) {
        j["value"] = idx;
        j["enum_id"] = enum_id;
        j["null"] = null;
      });
  return j;
}

void LiteralExpression::Accept(ExpressionVisitor& visitor) {
  return visitor.Visit(*this);
}

void LiteralExpression::Accept(ImmutableExpressionVisitor& visitor) const {
  return visitor.Visit(*this);
}

}  // namespace kush::plan