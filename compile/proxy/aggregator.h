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
  virtual ~Aggregator() = default;

  virtual void AddFields(StructBuilder& fields) = 0;
  virtual void Initialize(Struct& entry) = 0;
  virtual void Update(Struct& entry) = 0;
  virtual SQLValue Get(Struct& entry) = 0;
};

class SumAggregator : public Aggregator {
 public:
  SumAggregator(
      khir::ProgramBuilder& program,
      util::Visitor<plan::ImmutableExpressionVisitor, const plan::Expression&,
                    SQLValue>& expr_translator,
      const kush::plan::AggregateExpression& agg);
  virtual ~SumAggregator() = default;

  void AddFields(StructBuilder& fields) override;
  void Initialize(Struct& entry) override;
  void Update(Struct& entry) override;
  SQLValue Get(Struct& entry) override;

 private:
  khir::ProgramBuilder& program_;
  util::Visitor<plan::ImmutableExpressionVisitor, const plan::Expression&,
                SQLValue>& expr_translator_;
  const kush::plan::AggregateExpression& agg_;
  int field_;
};

class MinMaxAggregator : public Aggregator {
 public:
  MinMaxAggregator(
      khir::ProgramBuilder& program,
      util::Visitor<plan::ImmutableExpressionVisitor, const plan::Expression&,
                    SQLValue>& expr_translator,
      const kush::plan::AggregateExpression& agg, bool min);
  virtual ~MinMaxAggregator() = default;

  void AddFields(StructBuilder& fields) override;
  void Initialize(Struct& entry) override;
  void Update(Struct& entry) override;
  SQLValue Get(Struct& entry) override;

 private:
  khir::ProgramBuilder& program_;
  util::Visitor<plan::ImmutableExpressionVisitor, const plan::Expression&,
                SQLValue>& expr_translator_;
  const kush::plan::AggregateExpression& agg_;
  bool min_;
  int field_;
};

class AverageAggregator : public Aggregator {
 public:
  AverageAggregator(
      khir::ProgramBuilder& program,
      util::Visitor<plan::ImmutableExpressionVisitor, const plan::Expression&,
                    SQLValue>& expr_translator,
      const kush::plan::AggregateExpression& agg);
  virtual ~AverageAggregator() = default;

  void AddFields(StructBuilder& fields) override;
  void Initialize(Struct& entry) override;
  void Update(Struct& entry) override;
  SQLValue Get(Struct& entry) override;

 private:
  Float64 ToFloat(IRValue& v);

  khir::ProgramBuilder& program_;
  util::Visitor<plan::ImmutableExpressionVisitor, const plan::Expression&,
                SQLValue>& expr_translator_;
  const kush::plan::AggregateExpression& agg_;
  int value_field_;
  int count_field_;
};

class CountAggregator : public Aggregator {
 public:
  CountAggregator(
      khir::ProgramBuilder& program,
      util::Visitor<plan::ImmutableExpressionVisitor, const plan::Expression&,
                    SQLValue>& expr_translator,
      const kush::plan::AggregateExpression& agg);
  virtual ~CountAggregator() = default;

  void AddFields(StructBuilder& fields) override;
  void Initialize(Struct& entry) override;
  void Update(Struct& entry) override;
  SQLValue Get(Struct& entry) override;

 private:
  khir::ProgramBuilder& program_;
  util::Visitor<plan::ImmutableExpressionVisitor, const plan::Expression&,
                SQLValue>& expr_translator_;
  const kush::plan::AggregateExpression& agg_;
  int field_;
};

}  // namespace kush::compile::proxy