#include "compilation/cpp_translator.h"

#include "algebra/operator.h"
#include "compilation/translator_registry.h"

namespace skinner {

void ProduceScan(const TranslatorRegistry& registry, Operator& op,
                 std::ostream& out) {
  Scan& scan = static_cast<Scan&>(op);
  out << "for (tuple : " << scan.relation << ") {\n";

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
  out << "print(tuple)\n";
}

TranslatorRegistry GenerateCppRegistry() {
  TranslatorRegistry registry;
  registry.RegisterProducer(Scan::ID, &ProduceScan);
  registry.RegisterProducer(Select::ID, &ProduceSelect);
  registry.RegisterConsumer(Select::ID, &ConsumeSelect);
  registry.RegisterProducer(Output::ID, &ProduceOutput);
  registry.RegisterConsumer(Output::ID, &ConsumeOutput);
  return registry;
}

}  // namespace skinner