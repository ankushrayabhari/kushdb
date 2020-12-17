#pragma once

namespace kush::plan {

class ScanOperator;
class SelectOperator;
class OutputOperator;
class HashJoinOperator;

class OperatorVisitor {
 public:
  virtual void Visit(ScanOperator& scan) = 0;
  virtual void Visit(SelectOperator& select) = 0;
  virtual void Visit(OutputOperator& output) = 0;
  virtual void Visit(HashJoinOperator& hash_join) = 0;
};

}  // namespace kush::plan