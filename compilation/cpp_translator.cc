#include "compilation/cpp_translator.h"

#include <dlfcn.h>

#include <chrono>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <functional>
#include <string>
#include <type_traits>

#include "catalog/catalog.h"
#include "plan/expression/expression.h"
#include "plan/operator.h"
#include "plan/operator_schema.h"

namespace kush::compile {

using namespace plan;

std::string GenerateVar() {
  static std::string last = "a";
  last.back()++;
  if (last.back() > 'z') {
    last.back() = 'z';
    last.push_back('a');
  }
  return last;
}

std::string SqlTypeToRuntimeType(SqlType type) {
  switch (type) {
    case SqlType::INT:
      return "int32_t";
  }

  throw new std::runtime_error("Unknown type");
}

ProduceVisitor::ProduceVisitor(CppTranslator& translator)
    : translator_(translator) {}

void ProduceVisitor::Visit(Scan& scan) {
  if (!translator_.db_.contains(scan.relation)) {
    throw std::runtime_error("Unknown table: " + scan.relation);
  }

  const auto& table = translator_.db_[scan.relation];
  std::unordered_map<std::string, std::string> column_to_var;
  std::string card;

  for (const auto& column : scan.Schema().Columns()) {
    std::string var = GenerateVar();
    std::string type = SqlTypeToRuntimeType(column.Type());
    std::string path = table[std::string(column.Name())].path;
    column_to_var[std::string(column.Name())] = var;
    translator_.program_.fout << "kush::ColumnData<" << type << "> " << var
                              << "(\"" << path << "\");\n";

    if (card.empty()) {
      card = var + ".size()";
    }
  }

  std::string loop_var = GenerateVar();
  translator_.program_.fout << "for (uint32_t " << loop_var << " = 0; "
                            << loop_var << " < " << card << "; " << loop_var
                            << "++) {\n";

  std::vector<std::string> column_variables;

  for (const auto& column : scan.Schema().Columns()) {
    std::string type = SqlTypeToRuntimeType(column.Type());
    std::string column_var = column_to_var[std::string(column.Name())];
    std::string value_var = GenerateVar();

    column_variables.push_back(value_var);

    translator_.program_.fout << type << " " << value_var << " = " << column_var
                              << "[" << loop_var << "];\n";
  }

  translator_.context_.SetOutputVariables(scan, std::move(column_variables));

  auto parent = scan.Parent();
  if (parent.has_value()) {
    translator_.consumer_.Consume(parent.value(), scan);
  }

  translator_.program_.fout << "}\n";
}

void ProduceVisitor::Visit(Select& select) {
  translator_.producer_.Produce(select.Child());
}

void ProduceVisitor::Visit(Output& output) {
  translator_.producer_.Produce(output.Child());
}

void ProduceVisitor::Visit(HashJoin& hash_join) {}

void ProduceVisitor::Produce(Operator& target) { target.Accept(*this); }

void ConsumeVisitor::Visit(plan::Scan& scan) {
  throw std::runtime_error("Scan cannot consume tuples - leaf operator");
}

void ConsumeVisitor::Visit(plan::Select& select) {
  // TODO: generate code for select expression
  translator_.program_.fout << "if (false) {\n";

  auto parent = select.Parent();
  if (parent.has_value()) {
    translator_.consumer_.Consume(parent.value(), select);
  }

  translator_.program_.fout << "}\n";
}

void ConsumeVisitor::Visit(plan::Output& output) {
  bool first = true;
  const auto& variables =
      translator_.context_.GetOutputVariables(output.Child());
  if (variables.empty()) {
    return;
  }

  translator_.program_.fout << "std::cout";

  for (const auto& column_value_var : variables) {
    if (first) {
      translator_.program_.fout << " << " << column_value_var;
      first = false;
    } else {
      translator_.program_.fout << " << \",\" << " << column_value_var;
    }
  }

  translator_.program_.fout << ";\n";
}

void ConsumeVisitor::Visit(plan::HashJoin& hash_join) {}

void ConsumeVisitor::Consume(Operator& target, Operator& src) {
  src_.push(&src);
  target.Accept(*this);
  src_.pop();
}

Operator& ConsumeVisitor::GetSource() { return *src_.top(); }

/*
std::unordered_map<HashJoin*, std::string> hashjoin_buffer_var;
std::unordered_map<Operator*, std::vector<Column>> hj_cols;

void ProduceHashJoin(Context& ctx, Operator& op, std::ostream& out) {
  HashJoin& join = static_cast<HashJoin&>(op);
  if (join.expression->type != BinaryOperatorType::EQ) {
    throw std::runtime_error("Only support eq op for hash joins.");
  }
  auto left = dynamic_cast<ColumnRef*>(join.expression->left.get());
  auto right = dynamic_cast<ColumnRef*>(join.expression->right.get());
  if (left == nullptr || right == nullptr) {
    throw std::runtime_error("Only support column ref eq op hash joins.");
  }

  // TODO: replace this with a proper name
  std::string buffer_var = "buffer" + get_unique_loop_var();
  hashjoin_buffer_var[&join] = buffer_var;
  out << "std::unordered_map<int32_t, std::vector<int32_t>> " << buffer_var
      << ";\n";
  ctx.registry.GetProducer(join.left->Id())(ctx, *join.left, out);
  ctx.registry.GetProducer(join.right->Id())(ctx, *join.right, out);
}

void ConsumeHashJoin(Context& ctx, Operator& op, Operator& src,
                     std::vector<Column> columns, std::ostream& out) {
  HashJoin& join = static_cast<HashJoin&>(op);
  auto lref = dynamic_cast<ColumnRef*>(join.expression->left.get());
  auto rref = dynamic_cast<ColumnRef*>(join.expression->right.get());
  std::string buffer_var = hashjoin_buffer_var[&join];
  if (&src == join.left.get()) {
    hj_cols[join.left.get()] = columns;

    ColumnRef* ref = nullptr;
    if (ctx.col_to_var.find(lref->table + "_" + lref->column) !=
        ctx.col_to_var.end()) {
      ref = lref;
    } else {
      ref = rref;
    }
    std::string ref_var = ctx.col_to_var[ref->table + "_" + ref->column];

    for (Column c : columns) {
      auto var = ctx.col_to_var[c.name];
      // add to hash table
      out << buffer_var << "[" << ref_var << "].push_back(" << var << ");\n";
    }
  } else {
    ColumnRef* ref = nullptr;
    if (ctx.col_to_var.find(lref->table + "_" + lref->column) !=
        ctx.col_to_var.end()) {
      ref = lref;
    } else {
      ref = rref;
    }
    std::string var = ctx.col_to_var[ref->table + "_" + ref->column];

    auto loop_var = get_unique_loop_var();

    // probe hash table and start outputting tuples

    auto lcols = hj_cols[join.left.get()];

    auto bucket_var = get_unique_loop_var();
    out << "for (int " << bucket_var << " = 0; " << bucket_var << " < "
        << buffer_var << "[" << var << "]"
        << ".size()"
        << "; " << bucket_var << "+= " << lcols.size() << "){\n";

    int i = 0;
    for (Column c : lcols) {
      ctx.col_to_var[c.name] = c.name;
      out << c.type << " " << c.name << " = " << buffer_var << "[" << var << "]"
          << "[" << bucket_var << "+ " << i++ << "];\n";
    }

    std::vector<Column> newcols;
    newcols.insert(newcols.end(), lcols.begin(), lcols.end());
    newcols.insert(newcols.end(), columns.begin(), columns.end());
    if (join.parent != nullptr) {
      Operator& parent = *join.parent;
      ctx.registry.GetConsumer(parent.Id())(ctx, parent, join, newcols, out);
    }

    for (Column c : lcols) {
      ctx.col_to_var.erase(c.name);
    }

    out << "}\n";
  }
}

void ConsumeColumnRef(Context& ctx, plan::Expression& expr, std::ostream& out) {
  ColumnRef& ref = static_cast<ColumnRef&>(expr);
  out << ctx.col_to_var[ref.table + "_" + ref.column];
}

void ConsumeIntLiteral(Context& ctx, plan::Expression& expr,
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

void ConsumeBinaryExpression(Context& ctx, plan::Expression& expr,
                             std::ostream& out) {
  BinaryExpression& binop = static_cast<BinaryExpression&>(expr);
  out << "(";
  ctx.registry.GetExprConsumer(binop.left->Id())(ctx, *binop.left, out);
  out << op_to_cpp_op.at(binop.type);
  ctx.registry.GetExprConsumer(binop.right->Id())(ctx, *binop.right, out);
  out << ")";
}

CppTranslator::CppTranslator(const catalog::Database& db) : context_(db) {
  context_.registry.RegisterProducer(Scan::ID, &ProduceScan);
  context_.registry.RegisterProducer(Select::ID, &ProduceSelect);
  context_.registry.RegisterConsumer(Select::ID, &ConsumeSelect);
  context_.registry.RegisterProducer(Output::ID, &ProduceOutput);
  context_.registry.RegisterConsumer(Output::ID, &ConsumeOutput);
  context_.registry.RegisterProducer(HashJoin::ID, &ProduceHashJoin);
  context_.registry.RegisterConsumer(HashJoin::ID, &ConsumeHashJoin);
  context_.registry.RegisterExprConsumer(ColumnRef::ID, &ConsumeColumnRef);
  context_.registry.RegisterExprConsumer(BinaryExpression::ID,
                                         &ConsumeBinaryExpression);
  context_.registry.RegisterExprConsumer(IntLiteral::ID, &ConsumeIntLiteral);
}

void CppTranslator::Produce(Operator& op) {
  auto start = std::chrono::system_clock::now();
  std::string file_name("/tmp/test_generated.cpp");
  std::string dylib("/tmp/test_generated.so");
  std::string command = "clang++ -O3 --std=c++17 -I. -shared -fpic " +
                        file_name + " catalog/catalog.cc -o " + dylib;

  context_.col_to_var.clear();

  std::ofstream fout;
  fout.open(file_name);
  fout << "#include \"catalog/catalog.h\"\n";
  fout << "#include \"data/column_data.h\"\n";
  fout << "#include <iostream>\n";
  fout << "#include <cstdint>\n";
  fout << "#include <vector>\n";
  fout << "#include <unordered_map>\n";
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
  system("rm /tmp/test_generated.so");

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
*/
}  // namespace kush::compile