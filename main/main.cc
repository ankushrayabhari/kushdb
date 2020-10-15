#include <memory>
#include <string>

#include "algebra/operator.h"
#include "compilation/cpp_translator.h"

using namespace skinner;

int main() {
  CppTranslator translator;

  std::unique_ptr<Operator> op =
      std::make_unique<Output>(std::make_unique<Scan>("table"));

  translator.Produce(*op);

  return 0;
}
