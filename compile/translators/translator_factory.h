#pragma once

#include <memory>
#include <vector>

#include "absl/container/flat_hash_map.h"

#include "compile/proxy/column_data.h"
#include "compile/proxy/hash_table.h"
#include "compile/proxy/value/ir_value.h"
#include "compile/proxy/vector.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "execution/pipeline.h"
#include "execution/query_state.h"
#include "khir/program_builder.h"
#include "plan/operator/operator.h"
#include "plan/operator/operator_visitor.h"
#include "util/visitor.h"

namespace kush::compile {

class TranslatorFactory
    : public util::Visitor<plan::ImmutableOperatorVisitor,
                           const plan::Operator&,
                           std::unique_ptr<OperatorTranslator>> {
 public:
  TranslatorFactory(khir::ProgramBuilder& program,
                    execution::PipelineBuilder& pipeline_builder,
                    execution::QueryState& state);
  virtual ~TranslatorFactory() = default;

  void Visit(const plan::ScanOperator& scan) override;
  void Visit(const plan::SelectOperator& select) override;
  void Visit(const plan::ScanSelectOperator& scan_select) override;
  void Visit(const plan::SimdScanSelectOperator& scan_select) override;
  void Visit(const plan::OutputOperator& output) override;
  void Visit(const plan::SkinnerJoinOperator& skinner_join) override;
  void Visit(const plan::HashJoinOperator& hash_join) override;
  void Visit(const plan::CrossProductOperator& cross_product) override;
  void Visit(const plan::GroupByAggregateOperator& group_by_agg) override;
  void Visit(const plan::AggregateOperator& agg) override;
  void Visit(const plan::OrderByOperator& order_by) override;

 private:
  std::vector<std::unique_ptr<OperatorTranslator>> GetChildTranslators(
      const plan::Operator& current);
  khir::ProgramBuilder& program_;
  execution::PipelineBuilder& pipeline_builder_;
  execution::QueryState& state_;
};

}  // namespace kush::compile
