#pragma once

#include <string>
#include <string_view>

#include "parse/expression/expression.h"

namespace kush::parse {

class ColumnRefExpression : public Expression {
 public:
  ColumnRefExpression(std::string_view column_name,
                      std::string_view table_name = "");

 private:
  std::string column_name_;
  std::string table_name_;
};

}  // namespace kush::parse