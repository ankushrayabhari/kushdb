#pragma once

#include "catalog/catalog.h"
#include "compile/cpp/cpp_compilation_context.h"
#include "compile/program.h"
#include "plan/operator.h"

namespace kush::compile {

class CppTranslator {
 public:
  CppTranslator(const catalog::Database& db, const plan::Operator& op);

  void Translate();

  Program& Program();

 private:
  CppCompilationContext context_;
  const plan::Operator& op_;
};

}  // namespace kush::compile