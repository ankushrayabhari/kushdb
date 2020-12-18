#include "compile/translators/group_by_aggregate_translator.h"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "compile/compilation_context.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "compile/types.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/group_by_aggregate_operator.h"

namespace kush::compile {

GroupByAggregateTranslator::GroupByAggregateTranslator(
    plan::GroupByAggregateOperator& group_by_agg, CompilationContext& context,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(std::move(children)),
      group_by_agg_(group_by_agg),
      context_(context),
      expr_translator_(context, *this) {}

void GroupByAggregateTranslator::Produce() {
  auto& program = context_.Program();

  // declare packing struct of each output row
  packed_struct_id_ = program.GenerateVariable();
  program.fout << " struct " << packed_struct_id_ << " {\n";

  record_count_field_ = program.GenerateVariable();
  program.fout << "int64_t " << record_count_field_ << ";\n";

  // - include every group by column inside the struct
  for (const auto& group_by : group_by_agg_.GroupByExprs()) {
    auto field = program.GenerateVariable();
    auto type = SqlTypeToRuntimeType(group_by.get().Type());
    packed_group_by_field_type_.emplace_back(field, type);
    program.fout << type << " " << field << ";\n";
  }

  // - include every aggregate inside the struct
  for (const auto& agg : group_by_agg_.AggExprs()) {
    auto field = program.GenerateVariable();
    auto type = SqlTypeToRuntimeType(agg.get().Type());
    packed_agg_field_type_.emplace_back(field, type);
    program.fout << type << " " << field << ";\n";
  }
  program.fout << "};\n";

  // init hash table of group by columns
  hash_table_var_ = program.GenerateVariable();
  program.fout << "std::unordered_map<std::size_t, std::vector<"
               << packed_struct_id_ << ">> " << hash_table_var_ << ";\n";

  // populate hash table
  Child().Produce();

  // loop over elements of HT and output row
  auto bucket_var = program.GenerateVariable();
  program.fout << "for (auto& [_, " << bucket_var << "] : " << hash_table_var_
               << ") {\n";

  auto loop_var = program.GenerateVariable();
  program.fout << "for (int " << loop_var << " = 0; " << loop_var << " < "
               << bucket_var << ".size(); " << loop_var << "++){\n";

  // unpack group by/aggregates
  for (const auto& [field, type] : packed_group_by_field_type_) {
    auto var = program.GenerateVariable();
    virtual_values_.AddVariable(var, type);
    program.fout << type << "& " << var << " = " << bucket_var << "["
                 << loop_var << "]." << field << ";\n";
  }
  for (const auto& [field, type] : packed_agg_field_type_) {
    auto var = program.GenerateVariable();
    virtual_values_.AddVariable(var, type);
    program.fout << type << "& " << var << " = " << bucket_var << "["
                 << loop_var << "]." << field << ";\n";
  }

  // generate output variables
  for (const auto& column : group_by_agg_.Schema().Columns()) {
    auto var = program.GenerateVariable();
    auto type = SqlTypeToRuntimeType(column.Expr().Type());

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

void GroupByAggregateTranslator::Consume(OperatorTranslator& src) {
  auto& program = context_.Program();
  auto group_by_exprs = group_by_agg_.GroupByExprs();
  auto agg_exprs = group_by_agg_.AggExprs();

  auto hash_var = program.GenerateVariable();
  program.fout << "std::size_t " << hash_var << " = 0;";
  program.fout << "kush::util::HashCombine(" << hash_var;
  for (auto& group_by : group_by_exprs) {
    program.fout << ", ";
    expr_translator_.Produce(group_by.get());
  }
  program.fout << ");\n";

  auto bucket_var = program.GenerateVariable();
  program.fout << "auto& " << bucket_var << " = " << hash_table_var_ << "["
               << hash_var << "];\n";

  auto found_var = program.GenerateVariable();
  program.fout << "bool " << found_var << " = false;\n";

  // loop over bucket and check equality of the group by columns
  auto loop_var = program.GenerateVariable();
  program.fout << "for (int " << loop_var << " = 0; " << loop_var << " < "
               << bucket_var << ".size(); " << loop_var << "++){\n";
  // check if the key columns are actually equal
  program.fout << "if (";

  for (int i = 0; i < group_by_exprs.size(); i++) {
    if (i > 0) {
      program.fout << "&&";
    }

    const auto& [field, type] = packed_group_by_field_type_[i];

    program.fout << "(" << bucket_var << "[" << loop_var << "]." << field
                 << " == ";
    expr_translator_.Produce(group_by_exprs[i]);
    program.fout << ")";
  }

  program.fout << ") {\n";

  // Update aggregations
  for (int i = 0; i < packed_agg_field_type_.size(); i++) {
    const auto& [field, type] = packed_agg_field_type_[i];
    auto& agg = agg_exprs[i].get();

    switch (agg.AggType()) {
      case plan::AggregateType::SUM:
        program.fout << bucket_var << "[" << loop_var << "]." << field
                     << " += ";
        expr_translator_.Produce(agg.Child());
        program.fout << ";\n";
        break;
      case plan::AggregateType::AVG:
        // Use Knuth, The Art of Computer Programming Vol 2, section 4.2.2 to
        // compute running average in a numerically stable way
        program.fout << bucket_var << "[" << loop_var << "]." << field
                     << " += (";
        expr_translator_.Produce(agg.Child());
        program.fout << " - " << bucket_var << "[" << loop_var << "]." << field
                     << ") / " << bucket_var << "[" << loop_var << "]."
                     << record_count_field_ << ";\n";
        break;
      case plan::AggregateType::COUNT:
        program.fout << bucket_var << "[" << loop_var << "]." << field
                     << "++;\n";
        break;
    }
  }
  // increment agg counter field
  program.fout << bucket_var << "[" << loop_var << "]." << record_count_field_
               << "++;";

  program.fout << found_var << " = true;\n";
  program.fout << "break;\n";
  program.fout << "}}\n";

  // if not found insert new group
  program.fout << "if (!" << found_var << ") {\n";
  program.fout << bucket_var << ".push_back(" << packed_struct_id_ << "{2";
  for (auto& group_by : group_by_exprs) {
    program.fout << ",";
    expr_translator_.Produce(group_by.get());
  }
  for (auto& agg : agg_exprs) {
    // initialize agg
    program.fout << ", static_cast<" << SqlTypeToRuntimeType(agg.get().Type())
                 << ">(";

    switch (agg.get().AggType()) {
      case plan::AggregateType::SUM:
      case plan::AggregateType::AVG:
        expr_translator_.Produce(agg.get().Child());
        break;
      case plan::AggregateType::COUNT:
        program.fout << "1";
        break;
    }
    program.fout << ")";
  }
  program.fout << "});\n";
  program.fout << "}\n";
}

}  // namespace kush::compile