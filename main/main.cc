#include <memory>
#include <string>

#include "algebra/operator.h"
#include "catalog/catalog.h"
#include "compilation/cpp_translator.h"

using namespace kush;
using namespace kush::algebra;
using namespace kush::compile;
using namespace kush::catalog;

int main() {
  Database db;
  db.insert("table").insert("x1", "int32_t", "test.skdbcol");
  db.insert("table").insert("x2", "int32_t", "test.skdbcol");

  CppTranslator translator(db);

  std::unique_ptr<Operator> op = std::make_unique<Output>(
      std::make_unique<Select>(std::make_unique<Scan>("table"),
                               std::make_unique<BinaryExpression>(
                                   BinaryOperatorType::LT,
                                   std::make_unique<ColumnRef>("table", "x1"),
                                   std::make_unique<IntLiteral>(100000000))));

  translator.Produce(*op);

  return 0;
}
