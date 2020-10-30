#pragma once

namespace kush::plan {

class Scan;
class Select;
class Output;
class HashJoin;

class OperatorVisitor {
 public:
  virtual void Visit(Scan& scan) = 0;
  virtual void Visit(Select& select) = 0;
  virtual void Visit(Output& output) = 0;
  virtual void Visit(HashJoin& hash_join) = 0;
};

}  // namespace kush::plan