#pragma once

#include <memory>
#include <vector>

#include "compile/compilation_context.h"
#include "compile/translators/operator_translator.h"
#include "plan/operator_visitor.h"
#include "util/visitor.h"

namespace kush::compile {

class TranslatorFactory
    : public util::Visitor<plan::ImmutableOperatorVisitor,
                           std::unique_ptr<OperatorTranslator>> {
 public:
  TranslatorFactory(CompilationContext& context);
  virtual ~TranslatorFactory() = default;

  void Visit(const plan::ScanOperator& scan) override;
  void Visit(const plan::SelectOperator& select) override;
  void Visit(const plan::OutputOperator& output) override;
  void Visit(const plan::HashJoinOperator& hash_join) override;
  void Visit(const plan::GroupByAggregateOperator& group_by_agg) override;

  std::unique_ptr<OperatorTranslator> Produce(const plan::Operator& target);

 private:
  std::vector<std::unique_ptr<OperatorTranslator>> GetChildTranslators(
      const plan::Operator& current);
  CompilationContext& context_;
};

}  // namespace kush::compile
