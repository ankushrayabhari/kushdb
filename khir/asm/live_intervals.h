#pragma once

#include <vector>

#include "khir/program_builder.h"

namespace kush::khir {

class LiveInterval {
 public:
  LiveInterval(int start, int end, bool floating);
  void Extend(int);
  void SetFloatingPoint(bool);
  int Start() const;
  int End() const;
  bool Undef() const;
  bool IsFloatingPoint() const;

 private:
  int start_;
  int end_;
  bool floating_;
};

std::vector<LiveInterval> ComputeLiveIntervals(const Function& func,
                                               const TypeManager& manager);

}  // namespace kush::khir