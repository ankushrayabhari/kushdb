#pragma once

#include <string>
#include <utility>
#include <vector>

#include "compile/proxy/hash_table.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "plan/hash_join_operator.h"

namespace kush::compile {

template <typename T>
class SkinnerJoinTranslator : public OperatorTranslator<T> {
 public:
  SkinnerJoinTranslator(
      const plan::SkinnerJoinOperator& join, ProgramBuilder<T>& program,
      std::vector<std::unique_ptr<OperatorTranslator<T>>> children);
  virtual ~SkinnerJoinTranslator() = default;
  void Produce() override;
  void Consume(OperatorTranslator<T>& src) override;

 private:
  const plan::SkinnerJoinOperator& join_;
  ProgramBuilder<T>& program_;
  ExpressionTranslator<T> expr_translator_;
};

}  // namespace kush::compile