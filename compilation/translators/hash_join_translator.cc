#include "compilation/translators/hash_join_translator.h"

#include "compilation/compilation_context.h"
#include "compilation/translators/expression_translator.h"
#include "compilation/translators/operator_translator.h"
#include "compilation/types.h"
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

  // declare packing struct
  packed_struct_id_ = program.GenerateVariable();
  program.fout << " struct " << packed_struct_id_ << " {\n";
  for (const auto& column : hash_join_.LeftChild().Schema().Columns()) {
    auto field = program.GenerateVariable();
    auto type = SqlTypeToRuntimeType(column.Type());
    packed_struct_field_ids_.emplace_back(field, type);
    program.fout << type << " " << field << ";\n";
  }
  program.fout << "};\n";

  // declare hash table
  hash_table_var_ = program.GenerateVariable();
  program.fout << "std::unordered_map<std::size_t, std::vector<"
               << packed_struct_id_ << ">> " << hash_table_var_ << ";\n";

  LeftChild().Produce();
  RightChild().Produce();
}

void HashJoinTranslator::Consume(OperatorTranslator& src) {
  auto& program = context_.Program();
  auto& left_translator = LeftChild();
  auto& right_translator = RightChild();

  // Build side
  if (&src == &left_translator) {
    // hash key
    auto hash_var = program.GenerateVariable();
    program.fout << "std::size_t " << hash_var << " = 0;";
    program.fout << "kush::util::HashCombine(" << hash_var << ",";
    ExpressionTranslator key_translator(context_, left_translator);
    key_translator.Produce(hash_join_.LeftColumn());
    program.fout << ");\n";
    auto bucket_var = program.GenerateVariable();
    program.fout << "auto& " << bucket_var << " = " << hash_table_var_ << "["
                 << hash_var << "];\n";

    // pack tuple into table
    program.fout << bucket_var << ".push_back(" << packed_struct_id_ << "{";
    bool first = true;
    for (const auto& [variable, type] : left_translator.GetValues().Values()) {
      if (first) {
        first = false;
      } else {
        program.fout << ",";
      }
      program.fout << variable;
    }
    program.fout << "});\n";
    return;
  }

  // Probe Side
  // hash tuple
  auto hash_var = program.GenerateVariable();
  program.fout << "std::size_t " << hash_var << " = 0;";
  program.fout << "kush::util::HashCombine(" << hash_var << ",";
  ExpressionTranslator key_translator(context_, right_translator);
  key_translator.Produce(hash_join_.RightColumn());
  program.fout << ");\n";
  auto bucket_var = program.GenerateVariable();
  program.fout << "auto& " << bucket_var << " = " << hash_table_var_ << "["
               << hash_var << "];\n";

  // loop over bucket
  auto loop_var = program.GenerateVariable();
  program.fout << "for (int " << loop_var << " = 0; " << loop_var << " < "
               << bucket_var << ".size(); " << loop_var << "++){\n";

  // unpack tuple - reuse variables from build side loop
  const auto& left_schema_values = left_translator.GetValues().Values();
  for (int i = 0; i < left_schema_values.size(); i++) {
    const auto& [variable, type] = left_schema_values[i];
    const auto& [field_id, _] = packed_struct_field_ids_[i];

    program.fout << type << "& " << variable << " = " << bucket_var << "["
                 << loop_var << "]." << field_id << ";\n";
  }

  // check if the key columns are actually equal
  program.fout << "if (";
  ExpressionTranslator left_key_translator(context_, left_translator);
  left_key_translator.Produce(hash_join_.LeftColumn());
  program.fout << " == ";
  ExpressionTranslator right_key_translator(context_, right_translator);
  right_key_translator.Produce(hash_join_.RightColumn());
  program.fout << ") {";

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

  program.fout << "}}\n";
}

}  // namespace kush::compile