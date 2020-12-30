#pragma once

namespace kush::plan {

class ScanOperator;
class SelectOperator;
class OutputOperator;
class HashJoinOperator;
class GroupByAggregateOperator;
class OrderByOperator;
class CrossProductOperator;

class OperatorVisitor {
 public:
  virtual void Visit(ScanOperator& scan) = 0;
  virtual void Visit(SelectOperator& select) = 0;
  virtual void Visit(OutputOperator& output) = 0;
  virtual void Visit(HashJoinOperator& hash_join) = 0;
  virtual void Visit(GroupByAggregateOperator& group_by_agg) = 0;
  virtual void Visit(OrderByOperator& order_by) = 0;
  virtual void Visit(CrossProductOperator& cross_product) = 0;
};

class ImmutableOperatorVisitor {
 public:
  virtual void Visit(const ScanOperator& scan) = 0;
  virtual void Visit(const SelectOperator& select) = 0;
  virtual void Visit(const OutputOperator& output) = 0;
  virtual void Visit(const HashJoinOperator& hash_join) = 0;
  virtual void Visit(const GroupByAggregateOperator& group_by_agg) = 0;
  virtual void Visit(const OrderByOperator& order_by) = 0;
  virtual void Visit(const CrossProductOperator& cross_product) = 0;
};

}  // namespace kush::plan