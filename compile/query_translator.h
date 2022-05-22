#pragma once

#include <memory>

#include "catalog/catalog.h"
#include "compile/translators/operator_translator.h"
#include "execution/executable_query.h"
#include "plan/operator/operator.h"

namespace kush::compile {

execution::ExecutableQuery TranslateQuery(const plan::Operator& op);

}  // namespace kush::compile