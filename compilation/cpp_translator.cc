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

/*
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

void ConsumeVisitor::Visit(plan::Scan& scan) {
  throw std::runtime_error("Scan cannot consume tuples - leaf operator");
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

*/

CppTranslator::CppTranslator(const catalog::Database& db) : db_(db) {}

const catalog::Database& CppTranslator::Catalog() { return db_; }

CppProgram& CppTranslator::Program() { return program_; }

}  // namespace kush::compile