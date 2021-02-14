#pragma once

#include <memory>

#include "catalog/catalog.h"
#include "compile/program.h"
#include "plan/operator.h"

namespace kush::compile {

class QueryTranslator {
 public:
  QueryTranslator(const plan::Operator& op);
  std::unique_ptr<Program> Translate();

 private:
  const plan::Operator& op_;
};

}  // namespace kush::compile