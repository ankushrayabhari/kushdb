#include "parse/expression/literal_expression.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

#include "absl/time/civil_time.h"
#include "absl/time/time.h"

#include "parse/expression/expression.h"

namespace kush::parse {

template <class>
inline constexpr bool always_false_v = false;

LiteralExpression::LiteralExpression(int16_t value) : value_(value) {}

LiteralExpression::LiteralExpression(int32_t value) : value_(value) {}

LiteralExpression::LiteralExpression(int64_t value) : value_(value) {}

LiteralExpression::LiteralExpression(double value) : value_(value) {}

LiteralExpression::LiteralExpression(absl::CivilDay value) : value_(value) {}

LiteralExpression::LiteralExpression(std::string_view value)
    : value_(std::string(value)) {}

LiteralExpression::LiteralExpression(bool value) : value_(value) {}

void LiteralExpression::Visit(std::function<void(int16_t)> f1,
                              std::function<void(int32_t)> f2,
                              std::function<void(int64_t)> f3,
                              std::function<void(double)> f4,
                              std::function<void(std::string)> f5,
                              std::function<void(bool)> f6,
                              std::function<void(absl::CivilDay)> f7) const {
  std::visit(
      [&](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, int16_t>) {
          f1(arg);
        } else if constexpr (std::is_same_v<T, int32_t>) {
          f2(arg);
        } else if constexpr (std::is_same_v<T, int64_t>) {
          f3(arg);
        } else if constexpr (std::is_same_v<T, double>) {
          f4(arg);
        } else if constexpr (std::is_same_v<T, std::string>) {
          f5(arg);
        } else if constexpr (std::is_same_v<T, bool>) {
          f6(arg);
        } else if constexpr (std::is_same_v<T, absl::CivilDay>) {
          f7(arg);
        } else {
          static_assert(always_false_v<T>, "unreachable");
        }
      },
      value_);
}

}  // namespace kush::parse