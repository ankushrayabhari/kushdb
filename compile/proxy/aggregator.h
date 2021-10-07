#pragma once

#include <vector>

#include "compile/proxy/aggregator.h"
#include "compile/proxy/struct.h"
#include "compile/proxy/value/sql_value.h"
#include "khir/program_builder.h"
#include "plan/expression/aggregate_expression.h"
#include "util/visitor.h"

namespace kush::compile::proxy {

class Aggregator {
 public:
  virtual void AddFields(StructBuilder& fields) = 0;
  virtual void AddInitialEntry(std::vector<SQLValue>& values) = 0;
  virtual void Update(std::vector<SQLValue>& current_values, Struct& entry) = 0;
};

class SumAggregator : public Aggregator {
 public:
  SumAggregator(
      khir::ProgramBuilder& program,
      util::Visitor<plan::ImmutableExpressionVisitor, const plan::Expression&,
                    SQLValue>& expr_translator,
      const kush::plan::AggregateExpression& agg);
  void AddFields(StructBuilder& fields) override;
  void AddInitialEntry(std::vector<SQLValue>& values) override;
  void Update(std::vector<SQLValue>& current_values, Struct& entry) override;

 private:
  khir::ProgramBuilder& program_;
  util::Visitor<plan::ImmutableExpressionVisitor, const plan::Expression&,
                SQLValue>& expr_translator_;
  const kush::plan::AggregateExpression& agg_;
  int field_;
};

}  // namespace kush::compile::proxy