#include "compile/translators/group_by_aggregate_translator.h"

#include <string>
#include <utility>
#include <vector>

#include "compile/compilation_context.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "plan/group_by_aggregate_operator.h"

namespace kush::compile {

GroupByAggregateTranslator::GroupByAggregateTranslator(
    plan::GroupByAggregateOperator& group_by_agg, CompilationContext& context,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(std::move(children)),
      group_by_agg_(group_by_agg),
      context_(context),
      expr_translator_(context, *this) {}

void GroupByAggregateTranslator::Produce() {}

void GroupByAggregateTranslator::Consume(OperatorTranslator& src) {}

}  // namespace kush::compile