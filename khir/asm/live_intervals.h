#pragma once

#include <vector>

#include "khir/program_builder.h"

namespace kush::khir {

class LiveInterval {
 public:
  LiveInterval(khir::Value v, khir::Type t);
  void Extend(int);

  int Start() const;
  int End() const;
  bool Undef() const;
  khir::Type Type() const;
  khir::Value Value() const;

 private:
  int start_;
  int end_;
  khir::Value value_;
  khir::Type type_;
};

std::pair<std::vector<LiveInterval>, std::vector<int>> ComputeLiveIntervals(
    const Function& func, const TypeManager& manager);

}  // namespace kush::khir