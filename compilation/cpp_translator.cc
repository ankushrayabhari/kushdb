#include "compilation/cpp_translator.h"

#include <string>

#include "algebra/operator.h"
#include "compilation/translator_registry.h"

namespace skinner {

void ProduceScan(const TranslatorRegistry& registry, Operator& op,
                 std::ostream& out) {
  static int id = 0;
  std::string rel_name = "relation" + std::to_string(id);
  std::string val_name = "column" + std::to_string(id++);

  Scan& scan = static_cast<Scan&>(op);
  out << "skinner::ColumnData<int32_t> " << rel_name << "(\"test.skdbcol\");\n";

  out << "for (const auto& " << val_name << " : " << rel_name << ") {\n";
  if (scan.parent != nullptr) {
    Operator& parent = *scan.parent;
    registry.GetConsumer(parent.Id())(registry, parent, scan, out);
  }

  out << "}\n";
}

void ProduceSelect(const TranslatorRegistry& registry, Operator& op,
                   std::ostream& out) {
  Select& select = static_cast<Select&>(op);
  Operator& child = *select.child;
  registry.GetProducer(child.Id())(registry, child, out);
}

void ConsumeSelect(const TranslatorRegistry& registry, Operator& op,
                   Operator& src, std::ostream& out) {
  Select& select = static_cast<Select&>(op);
  out << "if (" << select.expression << ") {\n";

  if (select.parent != nullptr) {
    Operator& parent = *select.parent;
    registry.GetConsumer(parent.Id())(registry, parent, select, out);
  }

  out << "}\n";
}

void ProduceOutput(const TranslatorRegistry& registry, Operator& op,
                   std::ostream& out) {
  Output& select = static_cast<Output&>(op);
  Operator& child = *select.child;
  registry.GetProducer(child.Id())(registry, child, out);
}

void ConsumeOutput(const TranslatorRegistry& registry, Operator& op,
                   Operator& src, std::ostream& out) {
  out << "std::cout << column0 << std::endl;\n";
}

CppTranslator::CppTranslator() {
  registry_.RegisterProducer(Scan::ID, &ProduceScan);
  registry_.RegisterProducer(Select::ID, &ProduceSelect);
  registry_.RegisterConsumer(Select::ID, &ConsumeSelect);
  registry_.RegisterProducer(Output::ID, &ProduceOutput);
  registry_.RegisterConsumer(Output::ID, &ConsumeOutput);
}

void CppTranslator::Produce(Operator& op) {
  std::cout << "#include \"catalog/catalog.h\"\n";
  std::cout << "#include \"data/column_data.h\"\n";
  std::cout << "#include <iostream>\n";
  std::cout << "#include <cstdint>\n";
  std::cout << "void compute() {\n";
  registry_.GetProducer(op.Id())(registry_, op, std::cout);
  std::cout << "}\n";
}

}  // namespace skinner