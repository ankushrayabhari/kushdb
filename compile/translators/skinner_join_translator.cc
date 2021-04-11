#include "compile/translators/skinner_join_translator.h"

#include <string>
#include <utility>
#include <vector>

#include "compile/ir_registry.h"
#include "compile/proxy/hash_table.h"
#include "compile/proxy/if.h"
#include "compile/proxy/loop.h"
#include "compile/proxy/value.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "plan/hash_join_operator.h"
#include "util/vector_util.h"

namespace kush::compile {

template <typename T>
SkinnerJoinTranslator<T>::SkinnerJoinTranslator(
    const plan::SkinnerJoinOperator& join, ProgramBuilder<T>& program,
    std::vector<std::unique_ptr<OperatorTranslator<T>>> children)
    : OperatorTranslator<T>(std::move(children)),
      join_(join),
      program_(program),
      expr_translator_(program_, *this) {}

template <typename T>
void SkinnerJoinTranslator<T>::Produce() {}

template <typename T>
void SkinnerJoinTranslator<T>::Consume(OperatorTranslator<T>& src) {}

INSTANTIATE_ON_IR(SkinnerJoinTranslator);

}  // namespace kush::compile