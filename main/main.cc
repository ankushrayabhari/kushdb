#include <memory>
#include <string>

#include "algebra/operator.h"

using namespace skinner;

int main() {
  std::unique_ptr<algebra::Operator> select = std::make_unique<algebra::Select>(
      std::make_unique<algebra::TableScan>("table"), "x = 5");
  select->Print(std::cout);
  return 0;
}
