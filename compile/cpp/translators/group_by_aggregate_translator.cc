#include "compile/cpp/translators/group_by_aggregate_translator.h"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "compile/cpp/cpp_compilation_context.h"
#include "compile/cpp/translators/expression_translator.h"
#include "compile/cpp/translators/operator_translator.h"
#include "compile/cpp/types.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/group_by_aggregate_operator.h"

namespace kush::compile::cpp {

GroupByAggregateTranslator::GroupByAggregateTranslator(
    const plan::GroupByAggregateOperator& group_by_agg,
    CppCompilationContext& context,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(std::move(children)),
      group_by_agg_(group_by_agg),
      context_(context),
      expr_translator_(context, *this) {}

void GroupByAggregateTranslator::Produce() {
  auto& program = context_.Program();
  auto group_by_exprs = group_by_agg_.GroupByExprs();
  auto agg_exprs = group_by_agg_.AggExprs();

  if (!group_by_exprs.empty()) {
    // declare packing struct of each output row
    packed_struct_id_ = program.GenerateVariable();
    program.fout << " struct " << packed_struct_id_ << " {\n";
  }

  record_count_field_ = program.GenerateVariable();
  program.fout << "int64_t " << record_count_field_ << " = 0;\n";

  // - include every group by column inside the struct
  for (const auto& group_by : group_by_exprs) {
    auto field = program.GenerateVariable();
    auto type = SqlTypeToRuntimeType(group_by.get().Type());
    packed_group_by_field_type_.emplace_back(field, type);
    program.fout << type << " " << field << ";\n";
  }

  // - include every aggregate inside the struct
  for (const auto& agg : agg_exprs) {
    auto field = program.GenerateVariable();
    auto type = SqlTypeToRuntimeType(agg.get().Type());
    packed_agg_field_type_.emplace_back(field, type);
    program.fout << type << " " << field << ";\n";
  }

  if (!group_by_exprs.empty()) {
    program.fout << "};\n";
    hash_table_var_ = program.GenerateVariable();
    program.fout << "std::unordered_map<std::size_t, std::vector<"
                 << packed_struct_id_ << ">> " << hash_table_var_ << ";\n";
  }

  // populate hash table
  Child().Produce();

  if (group_by_exprs.empty()) {
    for (const auto& [field, type] : packed_agg_field_type_) {
      virtual_values_.AddVariable(field, type);
    }
    // check and make sure we have tuples
    program.fout << "if (" << record_count_field_ << " != 0) {\n";
  } else {
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
  }

  // generate output variables
  for (const auto& column : group_by_agg_.Schema().Columns()) {
    auto var = program.GenerateVariable();
    auto type = SqlTypeToRuntimeType(column.Expr().Type());

    program.fout << "auto " << var << " = "
                 << expr_translator_.Compute(column.Expr())->Get();
    program.fout << ";\n";

    values_.AddVariable(var, type);
  }

  if (auto parent = Parent()) {
    parent->get().Consume(*this);
  }

  if (group_by_exprs.empty()) {
    program.fout << "}\n";
  } else {
    program.fout << "}}\n";
  }
}

void GroupByAggregateTranslator::Consume(OperatorTranslator& src) {
  auto& program = context_.Program();
  auto group_by_exprs = group_by_agg_.GroupByExprs();
  auto agg_exprs = group_by_agg_.AggExprs();
  std::string struct_var, bucket_var, found_var, loop_var;
  if (group_by_exprs.empty()) {
    // check if uninit and if so init
    program.fout << "if (" << record_count_field_ << " == 0) {\n";
    program.fout << record_count_field_ << " = 2;\n";
    for (int i = 0; i < agg_exprs.size(); i++) {
      const auto& agg = agg_exprs[i];
      const auto& [field, type] = packed_agg_field_type_[i];
      // initialize agg
      program.fout << field << "= static_cast<"
                   << SqlTypeToRuntimeType(agg.get().Type()) << ">(";

      switch (agg.get().AggType()) {
        case plan::AggregateType::SUM:
        case plan::AggregateType::AVG:
        case plan::AggregateType::MAX:
        case plan::AggregateType::MIN:
          program.fout << expr_translator_.Compute(agg.get().Child())->Get();
          break;
        case plan::AggregateType::COUNT:
          program.fout << "1";
          break;
      }
      program.fout << ");\n";
    }
    program.fout << "} else {\n";
  } else {
    auto hash_var = program.GenerateVariable();
    program.fout << "std::size_t " << hash_var << " = 0;";
    program.fout << "kush::util::HashCombine(" << hash_var;
    for (auto& group_by : group_by_exprs) {
      program.fout << ", " << expr_translator_.Compute(group_by.get())->Get();
    }
    program.fout << ");\n";

    bucket_var = program.GenerateVariable();
    program.fout << "auto& " << bucket_var << " = " << hash_table_var_ << "["
                 << hash_var << "];\n";

    found_var = program.GenerateVariable();
    program.fout << "bool " << found_var << " = false;\n";

    // loop over bucket and check equality of the group by columns
    loop_var = program.GenerateVariable();
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
                   << " == "
                   << expr_translator_.Compute(group_by_exprs[i])->Get();
      program.fout << ")";
    }

    program.fout << ") {\n";
    struct_var = bucket_var + "[" + loop_var + "].";
  }

  // Update aggregations
  for (int i = 0; i < packed_agg_field_type_.size(); i++) {
    const auto& [field, type] = packed_agg_field_type_[i];
    auto& agg = agg_exprs[i].get();

    switch (agg.AggType()) {
      case plan::AggregateType::SUM:
        program.fout << struct_var << field
                     << " += " << expr_translator_.Compute(agg.Child())->Get();
        program.fout << ";\n";
        break;
      case plan::AggregateType::MIN:
        program.fout << struct_var << field << " = std::min("
                     << expr_translator_.Compute(agg.Child())->Get();
        program.fout << "," << struct_var << field << ");\n";
        break;
      case plan::AggregateType::MAX:
        program.fout << struct_var << field << " = std::max("
                     << expr_translator_.Compute(agg.Child())->Get();
        program.fout << "," << struct_var << field << ");\n";
        break;
      case plan::AggregateType::AVG:
        // Use Knuth, The Art of Computer Programming Vol 2, section 4.2.2 to
        // compute running average in a numerically stable way
        program.fout << struct_var << field << " += ("
                     << expr_translator_.Compute(agg.Child())->Get();
        program.fout << " - " << struct_var << field << ") / " << struct_var
                     << record_count_field_ << ";\n";
        break;
      case plan::AggregateType::COUNT:
        program.fout << struct_var << field << "++;\n";
        break;
    }
  }
  // increment agg counter field
  program.fout << struct_var << record_count_field_ << "++;";

  if (group_by_exprs.empty()) {
    program.fout << "}\n";
  } else {
    program.fout << found_var << " = true;\n";
    program.fout << "break;\n";
    program.fout << "}}\n";

    // if not found insert new group
    program.fout << "if (!" << found_var << ") {\n";
    program.fout << bucket_var << ".push_back(" << packed_struct_id_ << "{2";
    for (auto& group_by : group_by_exprs) {
      program.fout << "," << expr_translator_.Compute(group_by.get())->Get();
    }
    for (auto& agg : agg_exprs) {
      // initialize agg
      program.fout << ", static_cast<" << SqlTypeToRuntimeType(agg.get().Type())
                   << ">(";

      switch (agg.get().AggType()) {
        case plan::AggregateType::SUM:
        case plan::AggregateType::AVG:
        case plan::AggregateType::MAX:
        case plan::AggregateType::MIN:
          program.fout << expr_translator_.Compute(agg.get().Child())->Get();
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
}

}  // namespace kush::compile::cpp