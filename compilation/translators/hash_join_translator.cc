#include "compilation/translators/hash_join_translator.h"

#include "compilation/cpp_translator.h"
#include "compilation/translators/expression_translator.h"
#include "compilation/translators/operator_translator.h"
#include "plan/operator.h"

namespace kush::compile {

HashJoinTranslator::HashJoinTranslator(
    plan::HashJoin& hash_join, CppTranslator& context,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(std::move(children)),
      hash_join_(hash_join),
      context_(context) {}

void HashJoinTranslator::Produce() {
  auto& program = context_.Program();
  hash_table_var_ = program.GenerateVariable();
  program.fout << "std::unordered_map<int32_t, std::vector<int32_t>> "
               << hash_table_var_ << ";\n";

  LeftChild().Produce();
  RightChild().Produce();
}

void HashJoinTranslator::Consume(OperatorTranslator& src) {
  auto& program = context_.Program();
  auto& left_translator = LeftChild();
  auto& right_translator = RightChild();

  if (&src == &left_translator) {
    // TODO: hash key
    auto bucket_var = program.GenerateVariable();
    program.fout << "auto& " << bucket_var << " = " << hash_table_var_ << "[";
    ExpressionTranslator key_translator(context_, left_translator);
    key_translator.Produce(hash_join_.LeftColumn());
    program.fout << "];\n";

    // TODO: pack tuple into HT
    for (const auto& [variable, type] : left_translator.GetValues().Values()) {
      program.fout << bucket_var << ".push_back(" << variable << ");\n";
    }
  } else {
    /*
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

    out << "}\n"; */
  }
}

}  // namespace kush::compile