#pragma once
#include <iostream>
#include <stack>
#include <string>
#include <vector>

#include "catalog/catalog.h"
#include "compilation/cpp_program.h"
#include "plan/operator.h"
#include "plan/operator_visitor.h"

namespace kush::compile {

class CompilationContext {
 public:
  void SetOutputVariables(plan::Operator& op,
                          std::vector<std::string> column_variables);
  const std::vector<std::string>& GetOutputVariables(plan::Operator& op);

 private:
  std::unordered_map<plan::Operator*, std::vector<std::string>>
      operator_to_output_variables;
};

class CppTranslator {
 public:
  CppTranslator(const catalog::Database& db);

 private:
  CompilationContext context_;
  const catalog::Database& db_;
  CppProgram program_;
};

}  // namespace kush::compile
