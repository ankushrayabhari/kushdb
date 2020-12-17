#pragma once
#include <iostream>
#include <stack>
#include <string>
#include <vector>

#include "catalog/catalog.h"
#include "compile/cpp_program.h"
#include "plan/operator.h"
#include "plan/operator_visitor.h"

namespace kush::compile {

class CompilationContext {
 public:
  CompilationContext(const catalog::Database& db);
  const catalog::Database& Catalog();
  CppProgram& Program();

 private:
  const catalog::Database& db_;
  CppProgram program_;
};

}  // namespace kush::compile
