#pragma once

#include <string>
#include <utility>
#include <vector>

#include "compile/proxy/column_index.h"
#include "compile/proxy/struct.h"
#include "compile/proxy/vector.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "plan/skinner_join_operator.h"

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
  std::vector<std::unique_ptr<proxy::Vector<T>>> buffers_;
  std::vector<std::unique_ptr<proxy::ColumnIndex<T>>> indexes_;
  std::vector<std::reference_wrapper<const plan::ColumnRefExpression>>
      predicate_columns_;
  absl::flat_hash_map<std::pair<int, int>, int> predicate_to_index_idx_;
  int child_idx_ = -1;
};

}  // namespace kush::compile