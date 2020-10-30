#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include "catalog/catalog.h"
// #include "compilation/cpp_translator.h"
#include "nlohmann/json.hpp"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/comparison_expression.h"
#include "plan/expression/literal_expression.h"
#include "plan/operator.h"

using namespace kush;
using namespace kush::plan;
// using namespace kush::compile;
using namespace kush::catalog;

int main() {
  Database db;
  db.insert("table").insert("x1", "int32_t", "test.skdbcol");
  db.insert("table1").insert("x2", "int32_t", "test.skdbcol");
  db.insert("table2").insert("x3", "int32_t", "test.skdbcol");
  db.insert("table2").insert("x4", "int32_t", "test1.skdbcol");

  std::unique_ptr<Operator> op =
      std::make_unique<Output>(std::make_unique<HashJoin>(
          std::make_unique<Scan>("table2"),
          std::make_unique<HashJoin>(
              std::make_unique<Select>(
                  std::make_unique<Scan>("table"),
                  std::make_unique<ComparisonExpression>(
                      ComparisonType::LT,
                      std::make_unique<ColumnRefExpression>(0),
                      std::make_unique<LiteralExpression<int32_t>>(100000000))),
              std::make_unique<Scan>("table1"),
              std::make_unique<ColumnRefExpression>(0),
              std::make_unique<ColumnRefExpression>(1)),
          std::make_unique<ColumnRefExpression>(1),
          std::make_unique<ColumnRefExpression>(3)));

  std::cout << std::setw(2) << op->ToJson() << std::endl;

  return 0;
}
