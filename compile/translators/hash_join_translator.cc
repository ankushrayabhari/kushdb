#include "compile/translators/hash_join_translator.h"

#include <string>
#include <utility>
#include <vector>

#include "compile/ir_registry.h"
#include "compile/proxy/hash_table.h"
#include "compile/proxy/value.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "plan/hash_join_operator.h"

namespace kush::compile {

template <typename T>
HashJoinTranslator<T>::HashJoinTranslator(
    const plan::HashJoinOperator& hash_join, ProgramBuilder<T>& program,
    std::vector<std::unique_ptr<OperatorTranslator<T>>> children)
    : OperatorTranslator<T>(std::move(children)),
      hash_join_(hash_join),
      program_(program),
      expr_translator_(program_, *this) {}

template <typename T>
void HashJoinTranslator<T>::Produce() {
  /*
  // declare packing struct
  packed_struct_id_ = program.GenerateVariable();
  program.fout << " struct " << packed_struct_id_ << " {\n";
  for (const auto& column : hash_join_.LeftChild().Schema().Columns()) {
    auto field = program.GenerateVariable();
    auto type = SqlTypeToRuntimeType(column.Expr().Type());
    packed_struct_field_ids_.emplace_back(field, type);
    program.fout << type << " " << field << ";\n";
  }
  program.fout << "};\n";

  // declare hash table
  hash_table_var_ = program.GenerateVariable();
  program.fout << "std::unordered_map<std::size_t, std::vector<"
               << packed_struct_id_ << ">> " << hash_table_var_ << ";\n";

               */

  this->LeftChild().Produce();
  this->RightChild().Produce();
}

template <typename T>
void HashJoinTranslator<T>::Consume(OperatorTranslator<T>& src) {
  auto& left_translator = this->LeftChild();
  const auto left_columns = hash_join_.LeftColumns();
  const auto right_columns = hash_join_.RightColumns();

  // Build side
  if (&src == &left_translator) {
    // hash key
    std::vector<std::unique_ptr<proxy::Value<T>>> key_columns;
    for (const auto& left_key : left_columns) {
      key_columns.push_back(expr_translator_.Compute(left_key.get()));
    }
    auto entry = buffer_->Insert();
  }
  /*
    // pack tuple into table
    program.fout << bucket_var << ".push_back(" << packed_struct_id_ << "{";
    bool first = true;
    for (const auto& [variable, type] :
         left_translator.SchemaValues().Values()) {
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
  program.fout << "kush::util::HashCombine(" << hash_var;
  for (const auto& col : right_columns) {
    program.fout << "," << expr_translator_.Compute(col.get())->Get();
  }
  program.fout << ");\n";
  auto bucket_var = program.GenerateVariable();
  program.fout << "auto& " << bucket_var << " = " << hash_table_var_ << "["
               << hash_var << "];\n";

  // loop over bucket
  auto loop_var = program.GenerateVariable();
  program.fout << "for (int " << loop_var << " = 0; " << loop_var << " < "
               << bucket_var << ".size(); " << loop_var << "++){\n";

  // unpack tuple - reuse variables from build side loop
  const auto& left_schema_values = left_translator.SchemaValues().Values();
  for (int i = 0; i < left_schema_values.size(); i++) {
    const auto& [variable, type] = left_schema_values[i];
    const auto& [field_id, _] = packed_struct_field_ids_[i];

    program.fout << type << "& " << variable << " = " << bucket_var << "["
                 << loop_var << "]." << field_id << ";\n";
  }

  // check if the key columns are actually equal
  program.fout << "if (";
  for (int i = 0; i < left_columns.size(); i++) {
    if (i > 0) {
      program.fout << " && ";
    }

    program.fout << "("
                 << expr_translator_.Compute(left_columns[i].get())->Get()
                 << " == "
                 << expr_translator_.Compute(right_columns[i].get())->Get()
                 << ")";
  }
  program.fout << ") {";

  for (const auto& column : hash_join_.Schema().Columns()) {
    auto var = program.GenerateVariable();
    auto type = SqlTypeToRuntimeType(column.Expr().Type());

    program.fout << "auto " << var << " = "
                 << expr_translator_.Compute(column.Expr())->Get() << ";\n";

    values_.AddVariable(var, type);
  }

  if (auto parent = Parent()) {
    parent->get().Consume(*this);
  }

  program.fout << "}}\n";
  */
}

}  // namespace kush::compile