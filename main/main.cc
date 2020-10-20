#include <memory>
#include <string>

#include "algebra/operator.h"
#include "compilation/cpp_translator.h"

using namespace skinner;
using namespace skinner::algebra;
using namespace skinner::compile;

int main() {
  CppTranslator translator;

  std::unique_ptr<Operator> op =
      std::make_unique<Output>(std::make_unique<Select>(
          std::make_unique<Scan>("table"),
          std::make_unique<BinaryExpression>(
              BinaryOperatorType::LT, std::make_unique<ColumnRef>("table.x1"),
              std::make_unique<IntLiteral>(100000000))));

  translator.Produce(*op);

  return 0;
}
