#pragma once

#include <string>
#include <utility>
#include <vector>

#include "compile/compilation_cache.h"
#include "compile/program.h"
#include "compile/proxy/column_index.h"
#include "compile/proxy/struct.h"
#include "compile/proxy/vector.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "compile/translators/recompiling_join_translator.h"
#include "khir/program_builder.h"
#include "plan/skinner_join_operator.h"

namespace kush::compile {

class RecompilingSkinnerJoinTranslator : public OperatorTranslator,
                                         public RecompilingJoinTranslator {
 public:
  RecompilingSkinnerJoinTranslator(
      const plan::SkinnerJoinOperator& join, khir::ProgramBuilder& program,
      std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~RecompilingSkinnerJoinTranslator() = default;
  void Produce() override;
  void Consume(OperatorTranslator& src) override;
  ExecuteJoinFn CompileJoinOrder(const std::vector<int>& order) override;

 private:
  const plan::SkinnerJoinOperator& join_;
  khir::ProgramBuilder& program_;
  ExpressionTranslator expr_translator_;
  std::vector<proxy::Vector> buffers_;
  std::vector<std::unique_ptr<proxy::ColumnIndex>> indexes_;
  std::vector<std::reference_wrapper<const plan::ColumnRefExpression>>
      predicate_columns_;
  absl::flat_hash_map<std::pair<int, int>, int> predicate_to_index_idx_;
  int child_idx_ = -1;
  CompilationCache cache_;
};

}  // namespace kush::compile