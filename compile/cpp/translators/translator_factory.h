#pragma once

#include <memory>
#include <vector>

#include "compile/cpp/cpp_compilation_context.h"
#include "compile/cpp/translators/operator_translator.h"
#include "plan/operator_visitor.h"
#include "util/visitor.h"

namespace kush::compile::cpp {

class TranslatorFactory
    : public util::Visitor<const plan::Operator&,
                           plan::ImmutableOperatorVisitor,
                           std::unique_ptr<OperatorTranslator>> {
 public:
  TranslatorFactory(CppCompilationContext& context);
  virtual ~TranslatorFactory() = default;

  void Visit(const plan::ScanOperator& scan) override;
  void Visit(const plan::SelectOperator& select) override;
  void Visit(const plan::OutputOperator& output) override;
  void Visit(const plan::HashJoinOperator& hash_join) override;
  void Visit(const plan::GroupByAggregateOperator& group_by_agg) override;

 private:
  std::vector<std::unique_ptr<OperatorTranslator>> GetChildTranslators(
      const plan::Operator& current);
  CppCompilationContext& context_;
};

}  // namespace kush::compile::cpp
