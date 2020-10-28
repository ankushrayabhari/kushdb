#include <memory>
#include <string>

#include "catalog/catalog.h"
#include "compilation/cpp_translator.h"
#include "plan/operator.h"

using namespace kush;
using namespace kush::plan;
using namespace kush::compile;
using namespace kush::catalog;

int main() {
  Database db;
  db.insert("table").insert("x1", "int32_t", "test.skdbcol");
  db.insert("table1").insert("x2", "int32_t", "test.skdbcol");
  db.insert("table2").insert("x3", "int32_t", "test.skdbcol");
  db.insert("table2").insert("x4", "int32_t", "test1.skdbcol");

  CppTranslator translator(db);

  std::unique_ptr<Operator> op =
      std::make_unique<Output>(std::make_unique<HashJoin>(
          std::make_unique<Scan>("table2"),
          std::make_unique<HashJoin>(
              std::make_unique<Select>(
                  std::make_unique<Scan>("table"),
                  std::make_unique<BinaryExpression>(
                      BinaryOperatorType::LT,
                      std::make_unique<ColumnRef>("table", "x1"),
                      std::make_unique<IntLiteral>(100000000))),
              std::make_unique<Scan>("table1"),
              std::make_unique<BinaryExpression>(
                  BinaryOperatorType::EQ,
                  std::make_unique<ColumnRef>("table", "x1"),
                  std::make_unique<ColumnRef>("table1", "x2"))),
          std::make_unique<BinaryExpression>(
              BinaryOperatorType::EQ,
              std::make_unique<ColumnRef>("table1", "x2"),
              std::make_unique<ColumnRef>("table2", "x3"))));

  translator.Produce(*op);

  return 0;
}
