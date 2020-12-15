#pragma once

#include <memory>
#include <vector>

#include "compilation/compilation_context.h"
#include "compilation/translators/operator_translator.h"
#include "plan/operator_visitor.h"
#include "util/visitor.h"

namespace kush::compile {

class TranslatorFactory
    : public util::Visitor<plan::OperatorVisitor,
                           std::unique_ptr<OperatorTranslator>> {
 public:
  TranslatorFactory(CompilationContext& context);
  virtual ~TranslatorFactory() = default;

  void Visit(plan::Scan& scan) override;
  void Visit(plan::Select& select) override;
  void Visit(plan::Output& output) override;
  void Visit(plan::HashJoin& hash_join) override;

  std::unique_ptr<OperatorTranslator> Produce(plan::Operator& target);

 private:
  std::vector<std::unique_ptr<OperatorTranslator>> GetChildTranslators(
      plan::Operator& current);
  CompilationContext& context_;
};

}  // namespace kush::compile
