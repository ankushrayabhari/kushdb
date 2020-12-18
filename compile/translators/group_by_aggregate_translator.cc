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
  // -include every group by column inside the struct
  // -include every agg inside the struct
  // init hash table of group by columns

  Child().Produce();

  // loop over each bucket of HT
  // sort it
  // output 1 row per group
}

void GroupByAggregateTranslator::Consume(OperatorTranslator& src) {
  // store into HT
}

}  // namespace kush::compile