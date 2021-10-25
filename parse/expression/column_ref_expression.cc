#include "parse/expression/column_ref_expression.h"

#include <string>
#include <string_view>

#include "parse/expression/expression.h"

namespace kush::parse {

ColumnRefExpression::ColumnRefExpression(std::string_view column_name,
                                         std::string_view table_name)
    : column_name_(column_name), table_name_(table_name) {}

}  // namespace kush::parse