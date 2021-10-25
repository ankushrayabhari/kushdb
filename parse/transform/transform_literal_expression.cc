#include "parse/transform/transform_literal_expression.h"

#include <cassert>
#include <memory>
#include <vector>

#include "parse/expression/literal_expression.h"
#include "third_party/duckdb_libpgquery/parser.h"

namespace kush::parse {

std::unique_ptr<LiteralExpression> TransformLiteralExpression(
    duckdb_libpgquery::PGValue value) {
  switch (value.type) {
    case duckdb_libpgquery::T_PGInteger:
      assert(value.val.ival >= INT32_MIN && value.val.ival <= INT32_MAX);
      return std::make_unique<LiteralExpression>((int32_t)value.val.ival);

    case duckdb_libpgquery::T_PGString:
      return std::make_unique<LiteralExpression>(
          std::string_view(value.val.str));

    case duckdb_libpgquery::T_PGNull:
      return std::make_unique<LiteralExpression>(nullptr);

    case duckdb_libpgquery::T_PGFloat: {
      std::string str_val(value.val.str);
      bool try_cast_as_integer = true;
      int decimal_position = -1;
      for (int i = 0; i < str_val.size(); i++) {
        if (value.val.str[i] == '.') {
          try_cast_as_integer = false;
          decimal_position = i;
        }

        if (value.val.str[i] == 'e' || value.val.str[i] == 'E') {
          try_cast_as_integer = false;
        }
      }

      if (try_cast_as_integer) {
        int64_t bigint_value;

        try {
          bigint_value = std::stol(str_val);
          return std::make_unique<LiteralExpression>(bigint_value);
        } catch (...) {
        }
      } else {
        double real_value;

        try {
          real_value = std::stod(str_val);
          return std::make_unique<LiteralExpression>(real_value);
        } catch (...) {
        }
      }

      throw std::runtime_error("Invalid constant " + str_val);
    }

    case duckdb_libpgquery::T_PGBitString:
    default:
      throw std::runtime_error("Unsupported value type. " +
                               std::to_string(value.type));
  }
}

}  // namespace kush::parse