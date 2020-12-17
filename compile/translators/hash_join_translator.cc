#include "compile/translators/hash_join_translator.h"

#include "compile/compilation_context.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "compile/types.h"
#include "plan/hash_join_operator.h"

namespace kush::compile {

HashJoinTranslator::HashJoinTranslator(
    plan::HashJoinOperator& hash_join, CompilationContext& context,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(std::move(children)),
      hash_join_(hash_join),
      context_(context),
      expr_translator_(context, *this) {}

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

  // Build side
  if (&src == &left_translator) {
    // hash key
    auto hash_var = program.GenerateVariable();
    program.fout << "std::size_t " << hash_var << " = 0;";
    program.fout << "kush::util::HashCombine(" << hash_var << ",";
    expr_translator_.Produce(hash_join_.LeftColumn());
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
  expr_translator_.Produce(hash_join_.RightColumn());
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
  expr_translator_.Produce(hash_join_.LeftColumn());
  program.fout << " == ";
  expr_translator_.Produce(hash_join_.RightColumn());
  program.fout << ") {";

  for (const auto& column : hash_join_.Schema().Columns()) {
    auto var = program.GenerateVariable();
    auto type = SqlTypeToRuntimeType(column.Type());

    program.fout << "auto& " << var << " = ";
    expr_translator_.Produce(column.Expr());
    program.fout << ";\n";

    values_.AddVariable(var, type);
  }

  if (auto parent = Parent()) {
    parent->get().Consume(*this);
  }

  program.fout << "}}\n";
}

}  // namespace kush::compile