#include "compilation/cpp_translator.h"

#include <dlfcn.h>

#include <chrono>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <functional>
#include <string>
#include <type_traits>

#include "algebra/operator.h"
#include "compilation/translator_registry.h"

namespace skinner {

using namespace algebra;

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

  // TODO: generate code for expression
  out << "if (false) {\n";

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
  auto start = std::chrono::system_clock::now();
  std::string file_name("/tmp/test_generated.cpp");
  std::string dylib("/tmp/test_generated.so");
  std::string command = "clang++ --std=c++17 -I. -shared -fpic " + file_name +
                        " catalog/catalog.cc -o " + dylib;

  std::ofstream fout;
  fout.open(file_name);
  fout << "#include \"catalog/catalog.h\"\n";
  fout << "#include \"data/column_data.h\"\n";
  fout << "#include <iostream>\n";
  fout << "#include <cstdint>\n";
  fout << "extern \"C\" void compute() {\n";
  registry_.GetProducer(op.Id())(registry_, op, fout);
  fout << "}\n";
  fout.close();

  using compute_fn = std::add_pointer<void()>::type;
  auto gen = std::chrono::system_clock::now();

  if (system(command.c_str()) < 0) {
    throw std::runtime_error("Failed to execute command.");
  }
  auto comp = std::chrono::system_clock::now();

  void* handle = dlopen((dylib).c_str(), RTLD_LAZY);

  if (!handle) {
    throw std::runtime_error("Failed to open");
  }
  auto link = std::chrono::system_clock::now();

  auto process_query = (compute_fn)(dlsym(handle, "compute"));
  if (!process_query) {
    dlclose(handle);
    throw std::runtime_error("Failed to get compute fn");
  }

  process_query();
  dlclose(handle);

  std::cerr << "Performance stats (seconds):" << std::endl;
  auto end = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = gen - start;
  std::cerr << "Code generation: " << elapsed_seconds.count() << std::endl;
  elapsed_seconds = comp - gen;
  std::cerr << "Compilation: " << elapsed_seconds.count() << std::endl;
  elapsed_seconds = link - comp;
  std::cerr << "Linking: " << elapsed_seconds.count() << std::endl;
  elapsed_seconds = end - link;
  std::cerr << "Execution: " << elapsed_seconds.count() << std::endl;
}

}  // namespace skinner