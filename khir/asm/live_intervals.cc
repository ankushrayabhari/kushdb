#include "khir/asm/live_intervals.h"

#include <iostream>
#include <stack>
#include <vector>

#include "khir/program_builder.h"

namespace kush::khir {

LiveInterval::LiveInterval(int start, int end) : start_(start), end_(end) {}

int LiveInterval::Start() const { return start_; }

int LiveInterval::End() const { return end_; }

void RPO(int curr, const std::vector<std::vector<int>>& bb_succ,
         std::stack<int>& output, std::vector<bool>& visited) {
  for (auto succ : bb_succ[curr]) {
    if (!visited[succ]) {
      visited[succ] = true;
      RPO(succ, bb_succ, output, visited);
    }
  }
  output.push(curr);
}

std::vector<LiveInterval> ComputeLiveIntervals(const Function& func) {
  // Label each basic block using reverse post ordering
  const auto& bb = func.BasicBlocks();
  const auto& bb_succ = func.BasicBlockSuccessors();

  std::stack<int> temp;
  std::vector<bool> visited(bb.size(), false);
  visited[0] = true;
  RPO(0, bb_succ, temp, visited);

  std::vector<int> label(bb.size(), -1);
  int i = 0;
  while (!temp.empty()) {
    label[temp.top()] = i++;
    temp.pop();
  }

  return {};
}

}  // namespace kush::khir