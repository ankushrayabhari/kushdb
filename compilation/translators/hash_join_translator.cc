#include "compilation/translators/hash_join_translator.h"

#include "compilation/compilation_context.h"
#include "compilation/translators/expression_translator.h"
#include "compilation/translators/operator_translator.h"
#include "plan/operator.h"

namespace kush::compile {

HashJoinTranslator::HashJoinTranslator(
    plan::HashJoin& hash_join, CompilationContext& context,
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

  // Build side
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

    return;
  }

  // Probe Side
  auto bucket_var = program.GenerateVariable();
  program.fout << "auto& " << bucket_var << " = " << hash_table_var_ << "[";
  ExpressionTranslator key_translator(context_, right_translator);
  key_translator.Produce(hash_join_.RightColumn());
  program.fout << "];\n";

  const auto& left_schema_values = left_translator.GetValues().Values();

  auto loop_var = program.GenerateVariable();
  program.fout << "for (int " << loop_var << " = 0; " << loop_var << " < "
               << bucket_var << ".size(); " << loop_var
               << "+= " << left_schema_values.size() << "){\n";

  std::vector<std::string> column_variables;

  // TODO: unpack tuples from hash table
  int i = 0;
  for (const auto& [variable, type] : left_schema_values) {
    program.fout << type << "& " << variable << " = " << bucket_var << "["
                 << loop_var << "+ " << i++ << "];\n";
  }

  SchemaValues values;
  for (const auto& [variable, type] : left_schema_values) {
    values.AddVariable(variable, type);
  }
  const auto& right_schema_values = right_translator.GetValues().Values();
  for (const auto& [variable, type] : right_schema_values) {
    values.AddVariable(variable, type);
  }
  SetSchemaValues(std::move(values));

  if (auto parent = Parent()) {
    parent->get().Consume(*this);
  }

  program.fout << "}\n";
}

}  // namespace kush::compile