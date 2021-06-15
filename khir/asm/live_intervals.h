#pragma once

#include <vector>

#include "khir/program_builder.h"

namespace kush::khir {

class LiveInterval {
 public:
  LiveInterval(int start, int end);
  void Extend(int);
  int Start() const;
  int End() const;

 private:
  int start_;
  int end_;
};

std::vector<LiveInterval> ComputeLiveIntervals(const Function& func,
                                               const TypeManager& manager);

}  // namespace kush::khir