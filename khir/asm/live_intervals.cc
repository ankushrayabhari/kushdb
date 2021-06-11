#include "khir/asm/live_intervals.h"

#include <vector>

#include "khir/program_builder.h"

namespace kush::khir {

LiveInterval::LiveInterval(int start, int end) : start_(start), end_(end) {}

int LiveInterval::Start() const { return start_; }

int LiveInterval::End() const { return end_; }

std::vector<LiveInterval> ComputeLiveIntervals(const Function& func) {
  // compute the reverse post ordering of the basic blocks
  return {};
}

}  // namespace kush::khir