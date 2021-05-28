#pragma once

#include <string>
#include <utility>
#include <vector>

#include "compile/khir/khir_program_builder.h"
#include "compile/proxy/hash_table.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "plan/hash_join_operator.h"

namespace kush::compile {

class HashJoinTranslator : public OperatorTranslator {
 public:
  HashJoinTranslator(const plan::HashJoinOperator& hash_join,
                     khir::KHIRProgramBuilder& program,
                     std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~HashJoinTranslator() = default;
  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  const plan::HashJoinOperator& hash_join_;
  khir::KHIRProgramBuilder& program_;
  ExpressionTranslator expr_translator_;
  std::unique_ptr<proxy::HashTable> buffer_;
};

}  // namespace kush::compile