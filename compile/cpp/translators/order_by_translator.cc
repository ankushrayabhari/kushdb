#include "compile/cpp/translators/order_by_translator.h"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "compile/cpp/cpp_compilation_context.h"
#include "compile/cpp/translators/expression_translator.h"
#include "compile/cpp/translators/operator_translator.h"
#include "compile/cpp/types.h"
#include "plan/order_by_operator.h"

namespace kush::compile::cpp {

OrderByTranslator::OrderByTranslator(
    const plan::OrderByOperator& order_by, CppCompilationContext& context,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(std::move(children)),
      order_by_(order_by),
      context_(context),
      expr_translator_(context, *this) {}

void OrderByTranslator::Produce() {
  auto& program = context_.Program();

  Child().Produce();
  /*

  // declare packing struct of each input row
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
               << packed_struct_id_ << ">> " << hash_table_var_ << ";\n"; */

  // populate vector
  Child().Produce();

  // sort

  // output
}

void OrderByTranslator::Consume(OperatorTranslator& src) {
  // append elements to array
}

}  // namespace kush::compile::cpp