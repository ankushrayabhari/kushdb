#include "compilation/cpp_translator.h"

#include <dlfcn.h>

#include <chrono>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <functional>
#include <string>
#include <type_traits>

#include "algebra/expression.h"
#include "algebra/operator.h"
#include "compilation/translator_registry.h"

namespace skinner {
namespace compile {

using namespace algebra;
using Context = CppTranslator::CompilationContext;

void ProduceScan(Context& ctx, Operator& op, std::ostream& out) {
  Scan& scan = static_cast<Scan&>(op);
  // TODO: replace with catalog data
  out << "skinner::ColumnData<int32_t> table_x1(\"test.skdbcol\");\n";
  out << "uint32_t table_card = table_x1.size();\n";
  out << "for (int i = 0; i < table_card; i++) {\n";
  out << "int32_t x1 = table_x1[i];\n";
  ctx.col_to_var["table.x1"] = "x1";
  if (scan.parent != nullptr) {
    Operator& parent = *scan.parent;
    ctx.registry.GetConsumer(parent.Id())(ctx, parent, scan, out);
  }
  ctx.col_to_var.erase("table.x1");

  out << "}\n";
}

void ProduceSelect(Context& ctx, Operator& op, std::ostream& out) {
  Select& select = static_cast<Select&>(op);
  Operator& child = *select.child;
  ctx.registry.GetProducer(child.Id())(ctx, child, out);
}

void ConsumeSelect(Context& ctx, Operator& op, Operator& src,
                   std::ostream& out) {
  Select& select = static_cast<Select&>(op);

  out << "if (";
  ctx.registry.GetExprConsumer(select.expression->Id())(ctx, *select.expression,
                                                        out);
  out << ") {\n";

  if (select.parent != nullptr) {
    Operator& parent = *select.parent;
    ctx.registry.GetConsumer(parent.Id())(ctx, parent, select, out);
  }

  out << "}\n";
}

void ProduceOutput(Context& ctx, Operator& op, std::ostream& out) {
  Output& select = static_cast<Output&>(op);
  Operator& child = *select.child;
  ctx.registry.GetProducer(child.Id())(ctx, child, out);
}

void ConsumeOutput(Context& ctx, Operator& op, Operator& src,
                   std::ostream& out) {
  for (const auto& [_, var] : ctx.col_to_var) {
    out << "std::cout << " << var << " << std::endl;\n";
  }
}

void ConsumeColumnRef(Context& ctx, algebra::Expression& expr,
                      std::ostream& out) {
  ColumnRef& ref = static_cast<ColumnRef&>(expr);
  out << ctx.col_to_var[ref.column];
}

void ConsumeIntLiteral(Context& ctx, algebra::Expression& expr,
                       std::ostream& out) {
  IntLiteral& literal = static_cast<IntLiteral&>(expr);
  out << literal.value;
}

const std::unordered_map<BinaryOperatorType, std::string> op_to_cpp_op{
    {BinaryOperatorType::ADD, "+"}, {BinaryOperatorType::AND, "&&"},
    {BinaryOperatorType::DIV, "/"}, {BinaryOperatorType::EQ, "=="},
    {BinaryOperatorType::GT, ">"},  {BinaryOperatorType::GTE, ">="},
    {BinaryOperatorType::LT, "<"},  {BinaryOperatorType::LTE, "<="},
    {BinaryOperatorType::MUL, "*"}, {BinaryOperatorType::NEQ, "!="},
    {BinaryOperatorType::OR, "||"}, {BinaryOperatorType::SUB, "-"},
    {BinaryOperatorType::MOD, "%"}, {BinaryOperatorType::XOR, "^"},
};

void ConsumeBinaryExpression(Context& ctx, algebra::Expression& expr,
                             std::ostream& out) {
  BinaryExpression& binop = static_cast<BinaryExpression&>(expr);
  out << "(";
  ctx.registry.GetExprConsumer(binop.left->Id())(ctx, *binop.left, out);
  out << op_to_cpp_op.at(binop.type);
  ctx.registry.GetExprConsumer(binop.right->Id())(ctx, *binop.right, out);
  out << ")";
}

CppTranslator::CppTranslator() {
  context_.registry.RegisterProducer(Scan::ID, &ProduceScan);
  context_.registry.RegisterProducer(Select::ID, &ProduceSelect);
  context_.registry.RegisterConsumer(Select::ID, &ConsumeSelect);
  context_.registry.RegisterProducer(Output::ID, &ProduceOutput);
  context_.registry.RegisterConsumer(Output::ID, &ConsumeOutput);
  context_.registry.RegisterExprConsumer(ColumnRef::ID, &ConsumeColumnRef);
  context_.registry.RegisterExprConsumer(BinaryExpression::ID,
                                         &ConsumeBinaryExpression);
  context_.registry.RegisterExprConsumer(IntLiteral::ID, &ConsumeIntLiteral);
}

void CppTranslator::Produce(Operator& op) {
  auto start = std::chrono::system_clock::now();
  std::string file_name("/tmp/test_generated.cpp");
  std::string dylib("/tmp/test_generated.so");
  std::string command = "clang++ --std=c++17 -I. -shared -fpic " + file_name +
                        " catalog/catalog.cc -o " + dylib;

  context_.col_to_var.clear();

  std::ofstream fout;
  fout.open(file_name);
  fout << "#include \"catalog/catalog.h\"\n";
  fout << "#include \"data/column_data.h\"\n";
  fout << "#include <iostream>\n";
  fout << "#include <cstdint>\n";
  fout << "extern \"C\" void compute() {\n";
  context_.registry.GetProducer(op.Id())(context_, op, fout);
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

}  // namespace compile
}  // namespace skinner