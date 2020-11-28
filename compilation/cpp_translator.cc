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
#include "compilation/program.h"
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

std::unordered_map<HashJoin*, std::string> hashjoin_buffer_var;

void ProduceVisitor::Visit(HashJoin& hash_join) {
  std::string hash_table_var = GenerateVar();
  hashjoin_buffer_var[&hash_join] = hash_table_var;
  translator_.program_.fout
      << "std::unordered_map<int32_t, std::vector<int32_t>> " << hash_table_var
      << ";\n";
  translator_.producer_.Produce(hash_join.LeftChild());
  translator_.producer_.Produce(hash_join.RightChild());
}

void ProduceVisitor::Produce(Operator& target) { target.Accept(*this); }

ConsumeVisitor::ConsumeVisitor(CppTranslator& translator)
    : translator_(translator) {}

void ConsumeVisitor::Visit(plan::Scan& scan) {
  throw std::runtime_error("Scan cannot consume tuples - leaf operator");
}

void ConsumeVisitor::Visit(plan::Select& select) {
  // TODO: generate code for select expression
  translator_.program_.fout << "if (true) {\n";

  translator_.context_.SetOutputVariables(
      select, translator_.context_.GetOutputVariables(select.Child()));

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

  translator_.program_.fout << " << \"\\n\";\n";
}

void ConsumeVisitor::Visit(plan::HashJoin& hash_join) {
  std::string buffer_var = hashjoin_buffer_var[&hash_join];
  auto& src = translator_.consumer_.GetSource();
  auto& out = translator_.program_.fout;

  if (&src == &hash_join.LeftChild()) {
    auto& left_child = hash_join.LeftChild();
    std::string key_var = translator_.context_.GetOutputVariables(
        left_child)[hash_join.left_column_->GetColumnIdx()];

    // TODO: hash key
    auto bucket_var = GenerateVar();
    out << "auto& " << bucket_var << " = " << buffer_var << "[" << key_var
        << "];\n";

    for (const auto& column_value_var :
         translator_.context_.GetOutputVariables(left_child)) {
      // TODO: pack tuple into HT
      out << bucket_var << ".push_back(" << column_value_var << ");\n";
    }
  } else {
    auto& right_child = hash_join.RightChild();
    std::string key_var = translator_.context_.GetOutputVariables(
        right_child)[hash_join.right_column_->GetColumnIdx()];

    // probe hash table and start outputting tuples
    auto bucket_var = GenerateVar();
    out << "auto& " << bucket_var << " = " << buffer_var << "[" << key_var
        << "];\n";

    const auto& left_schema = hash_join.LeftChild().Schema().Columns();

    auto loop_var = GenerateVar();
    out << "for (int " << loop_var << " = 0; " << loop_var << " < "
        << bucket_var << ".size(); " << loop_var << "+= " << left_schema.size()
        << "){\n";

    std::vector<std::string> column_variables;

    // TODO: unpack tuples from hash table
    int i = 0;
    for (const auto& left_column : left_schema) {
      std::string var = GenerateVar();
      std::string type = SqlTypeToRuntimeType(left_column.Type());

      out << type << " " << var << " = " << bucket_var << "[" << loop_var
          << "+ " << i++ << "];\n";

      column_variables.push_back(var);
    }

    for (const auto& var :
         translator_.context_.GetOutputVariables(hash_join.RightChild())) {
      column_variables.push_back(var);
    }

    translator_.context_.SetOutputVariables(hash_join,
                                            std::move(column_variables));

    auto parent = hash_join.Parent();
    if (parent.has_value()) {
      translator_.consumer_.Consume(parent.value(), hash_join);
    }

    out << "}\n";
  }
}

void ConsumeVisitor::Consume(Operator& target, Operator& src) {
  src_.push(&src);
  target.Accept(*this);
  src_.pop();
}

Operator& ConsumeVisitor::GetSource() { return *src_.top(); }

void CompilationContext::SetOutputVariables(
    plan::Operator& op, std::vector<std::string> column_variables) {
  operator_to_output_variables.emplace(&op, std::move(column_variables));
}

const std::vector<std::string>& CompilationContext::GetOutputVariables(
    plan::Operator& op) {
  return operator_to_output_variables.at(&op);
}

CppTranslator::CppTranslator(const catalog::Database& db)
    : producer_(*this), consumer_(*this), db_(db) {}

Program& CppTranslator::Translate(plan::Operator& op) {
  program_.CodegenInitialize();
  producer_.Produce(op);
  program_.CodegenFinalize();
  return program_;
}

/*
void ConsumeColumnRef(Context& ctx, plan::Expression& expr, std::ostream& out) {
  ColumnRef& ref = static_cast<ColumnRef&>(expr);
  out << ctx.col_to_var[ref.table + "_" + ref.column];
}

void ConsumeIntLiteral(Context& ctx, plan::Expression& expr,
                       std::ostream& out) {
  IntLiteral& literal = static_cast<IntLiteral&>(expr);
  out << literal.value;
}

void ConsumeBinaryExpression(Context& ctx, plan::Expression& expr,
                             std::ostream& out) {
  BinaryExpression& binop = static_cast<BinaryExpression&>(expr);
  out << "(";
  ctx.registry.GetExprConsumer(binop.left->Id())(ctx, *binop.left, out);
  out << op_to_cpp_op.at(binop.type);
  ctx.registry.GetExprConsumer(binop.right->Id())(ctx, *binop.right, out);
  out << ")";
}
*/
}  // namespace kush::compile