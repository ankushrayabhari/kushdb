#pragma once
#include <unordered_map>

#include "algebra/operator.h"

namespace skinner {
namespace compile {

void Produce(algebra::Operator& op, std::ostream& out) {}

void Produce(algebra::Operator& op, std::ostream& out) {}

class TableScan final : public Operator {
 public:
  std::string name_;
  std::string relation_;
  TableScan(absl::string_view relation);
  void Print(std::ostream& out, int num_indent) const;
};

class Select final : public Operator {
 public:
  std::string name_;
  std::string expression_;
  std::unique_ptr<Operator> child_;
  Select(std::unique_ptr<Operator> child, absl::string_view expression);
  void Print(std::ostream& out, int num_indent) const;
};

}  // namespace compile
}  // namespace skinner