#pragma once

#include <cstdint>

namespace kush::plan {

class AggregateExpression;
class ColumnRefExpression;
class LiteralExpression;
class VirtualColumnRefExpression;
class UnaryArithmeticExpression;
class BinaryArithmeticExpression;
class CaseExpression;
class IntToFloatConversionExpression;
class ExtractExpression;

class ExpressionVisitor {
 public:
  virtual ~ExpressionVisitor() = default;
  virtual void Visit(UnaryArithmeticExpression& arith) = 0;
  virtual void Visit(BinaryArithmeticExpression& arith) = 0;
  virtual void Visit(AggregateExpression& agg) = 0;
  virtual void Visit(ColumnRefExpression& col_ref) = 0;
  virtual void Visit(VirtualColumnRefExpression& virtual_col_ref) = 0;
  virtual void Visit(LiteralExpression& literal) = 0;
  virtual void Visit(CaseExpression& case_expr) = 0;
  virtual void Visit(IntToFloatConversionExpression& conv) = 0;
  virtual void Visit(ExtractExpression& extract) = 0;
};

class ImmutableExpressionVisitor {
 public:
  virtual ~ImmutableExpressionVisitor() = default;
  virtual void Visit(const UnaryArithmeticExpression& arith) = 0;
  virtual void Visit(const BinaryArithmeticExpression& arith) = 0;
  virtual void Visit(const AggregateExpression& agg) = 0;
  virtual void Visit(const ColumnRefExpression& col_ref) = 0;
  virtual void Visit(const VirtualColumnRefExpression& virtual_col_ref) = 0;
  virtual void Visit(const LiteralExpression& literal) = 0;
  virtual void Visit(const CaseExpression& case_expr) = 0;
  virtual void Visit(const IntToFloatConversionExpression& conv) = 0;
  virtual void Visit(const ExtractExpression& extract) = 0;
};

}  // namespace kush::plan