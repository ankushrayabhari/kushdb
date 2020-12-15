#pragma once

#include "catalog/catalog.h"
#include "compilation/compilation_context.h"
#include "compilation/program.h"
#include "plan/operator.h"

namespace kush::compile {

class CppTranslator {
 public:
  CppTranslator(const catalog::Database& db, plan::Operator& op);
  void Translate();
  Program& Program();

 private:
  CompilationContext context_;
  plan::Operator& op_;
};

}  // namespace kush::compile