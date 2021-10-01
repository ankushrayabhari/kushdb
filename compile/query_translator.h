#pragma once

#include <memory>

#include "catalog/catalog.h"
#include "compile/translators/operator_translator.h"
#include "execution/executable_query.h"
#include "khir/program.h"
#include "plan/operator.h"

namespace kush::compile {

class QueryTranslator {
 public:
  QueryTranslator(const plan::Operator& op);
  execution::ExecutableQuery Translate();

 private:
  const plan::Operator& op_;
};

}  // namespace kush::compile