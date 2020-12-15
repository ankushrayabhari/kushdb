#include "compilation/translators/hash_join_translator.h"

#include "compilation/cpp_translator.h"
#include "compilation/translators/operator_translator.h"
#include "plan/operator.h"

namespace kush::compile {

HashJoinTranslator::HashJoinTranslator(
    plan::HashJoin& hash_join, CppTranslator& context,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(std::move(children)),
      hash_join_(hash_join),
      context_(context) {}

void HashJoinTranslator::Produce() {}

void HashJoinTranslator::Consume(OperatorTranslator& src) {}

}  // namespace kush::compile