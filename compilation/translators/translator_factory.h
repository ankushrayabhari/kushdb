#pragma once

#include "plan/operator_visitor.h"

class TranslatorFactory : public plan::OperatorVisitor {
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