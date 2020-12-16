#pragma once

#include <string>
#include <utility>
#include <vector>

#include "compilation/compilation_context.h"
#include "compilation/translators/operator_translator.h"
#include "plan/operator.h"

namespace kush::compile {

class HashJoinTranslator : public OperatorTranslator {
 public:
  HashJoinTranslator(plan::HashJoin& hash_join, CompilationContext& context,
                     std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~HashJoinTranslator() = default;
  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  std::string hash_table_var_;
  std::string packed_struct_id_;
  std::vector<std::pair<std::string, std::string>> packed_struct_field_ids_;

  plan::HashJoin& hash_join_;
  CompilationContext& context_;
};

}  // namespace kush::compile