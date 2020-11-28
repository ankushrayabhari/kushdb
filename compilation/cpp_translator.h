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

class CppTranslator;

class ProduceVisitor : public plan::OperatorVisitor {
 public:
  ProduceVisitor(CppTranslator& translator);

  void Visit(plan::Scan& scan) override;
  void Visit(plan::Select& select) override;
  void Visit(plan::Output& output) override;
  void Visit(plan::HashJoin& hash_join) override;

  void Produce(plan::Operator& target);

 private:
  CppTranslator& translator_;
};

class ConsumeVisitor : public plan::OperatorVisitor {
 public:
  ConsumeVisitor(CppTranslator& translator);

  void Visit(plan::Scan& scan) override;
  void Visit(plan::Select& select) override;
  void Visit(plan::Output& output) override;
  void Visit(plan::HashJoin& hash_join) override;

  void Consume(plan::Operator& target, plan::Operator& src);

 private:
  plan::Operator& GetSource();
  std::stack<plan::Operator*> src_;
  CppTranslator& translator_;
};

class CompilationContext {
 public:
  void SetOutputVariables(plan::Operator& op,
                          std::vector<std::string> column_variables);

 private:
  std::unordered_map<plan::Operator*, std::vector<std::string>>
      operator_to_output_variables;
};

class CppTranslator {
 public:
  CppTranslator(const catalog::Database& db);
  CppProgram& Produce(plan::Operator& op);

 private:
  ProduceVisitor producer_;
  ConsumeVisitor consumer_;
  CompilationContext context_;
  const catalog::Database& db_;
  CppProgram program_;

  friend class ProduceVisitor;
  friend class ConsumeVisitor;
};

}  // namespace kush::compile
