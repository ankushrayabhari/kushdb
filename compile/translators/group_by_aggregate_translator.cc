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
  program.fout << "for (const auto& [_, " << bucket_var
               << "] : " << hash_table_var_ << " {\n";

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
  // store into HT
}

}  // namespace kush::compile