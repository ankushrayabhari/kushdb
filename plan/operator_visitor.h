#pragma once

namespace kush::plan {

class ScanOperator;
class Select;
class OutputOperator;
class HashJoin;

class OperatorVisitor {
 public:
  virtual void Visit(ScanOperator& scan) = 0;
  virtual void Visit(Select& select) = 0;
  virtual void Visit(OutputOperator& output) = 0;
  virtual void Visit(HashJoin& hash_join) = 0;
};

}  // namespace kush::plan