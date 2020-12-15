#pragma once

#include <memory>
#include <vector>

#include "compilation/cpp_translator.h"
#include "compilation/translators/translator.h"
#include "plan/operator_visitor.h"

namespace kush::compile {

class TranslatorFactory : public plan::OperatorVisitor {
 public:
  TranslatorFactory(CppTranslator& context);

  void Visit(plan::Scan& scan) override;
  void Visit(plan::Select& select) override;
  void Visit(plan::Output& output) override;
  void Visit(plan::HashJoin& hash_join) override;

  void Produce(plan::Operator& target);

 private:
  void Return(std::unique_ptr<Translator> result);
  std::unique_ptr<Translator> GetResult();
  std::vector<std::unique_ptr<Translator>> GetChildTranslators(
      plan::Operator& current);
  std::unique_ptr<Translator> result_;
  CppTranslator& context_;
};

}  // namespace kush::compile
