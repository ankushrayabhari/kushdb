#pragma once

#include <string>
#include <utility>
#include <vector>

#include "compile/llvm/llvm_program.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "plan/cross_product_operator.h"

namespace kush::compile {

class CrossProductTranslator : public OperatorTranslator {
 public:
  CrossProductTranslator(
      const plan::CrossProductOperator& cross_product,
      CppCompilationContext& context,
      std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~CrossProductTranslator() = default;
  void Produce() override;
  void Consume(OperatorTranslator& src) override;

 private:
  const plan::CrossProductOperator& cross_product_;
  CppCompilationContext& context_;
  ExpressionTranslator expr_translator_;

  std::string hash_table_var_;
  std::string packed_struct_id_;
  std::vector<std::pair<std::string, std::string>> packed_struct_field_ids_;
};

}  // namespace kush::compile