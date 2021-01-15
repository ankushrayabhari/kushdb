#pragma once

#include "catalog/catalog.h"
#include "compile/program.h"
#include "plan/operator.h"

namespace kush::compile {

class QueryTranslator {
 public:
  QueryTranslator(const catalog::Database& db, const plan::Operator& op);
  Program Translate();

 private:
  const catalog::Database& db_;
  const plan::Operator& op_;
};

}  // namespace kush::compile