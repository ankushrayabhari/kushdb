#pragma once

#include <string>

#include "compilation/compilation_context.h"
#include "compilation/translators/operator_translator.h"
#include "plan/operator.h"

namespace kush::compile {

class HashJoinTranslator : public OperatorTranslator {
 public:
  HashJoinTranslator(plan::HashJoin& hash_join, CompliationContext& context,
                     std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~HashJoinTranslator() = default;
  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  std::string hash_table_var_;
  plan::HashJoin& hash_join_;
  CompliationContext& context_;
};

}  // namespace kush::compile