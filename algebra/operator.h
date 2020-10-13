#pragma once
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/strings/string_view.h"

namespace skinner {
namespace algebra {

class Operator {
 public:
  virtual ~Operator() {}
  virtual void Print(std::ostream& out, int num_indent = 0) const = 0;
};

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

}  // namespace algebra
}  // namespace skinner