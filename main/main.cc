#include <memory>
#include <string>

#include "algebra/operator.h"
#include "compilation/cpp_translator.h"

using namespace skinner;

int main() {
  const auto registry = GenerateCppRegistry();

  std::unique_ptr<Operator> op = std::make_unique<Output>(
      std::make_unique<Select>(std::make_unique<Scan>("table"), "x = 5"));

  registry.GetProducer(op->Id())(registry, *op, std::cout);

  return 0;
}
