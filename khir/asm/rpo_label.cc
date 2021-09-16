#include "khir/asm/rpo_label.h"

#include <stack>
#include <vector>

namespace kush::khir {

void RPOHelper(int curr, const std::vector<std::vector<int>>& bb_succ,
               std::stack<int>& output, std::vector<bool>& visited) {
  visited[curr] = true;

  const auto& successors = bb_succ[curr];
  for (int i = successors.size() - 1; i >= 0; i--) {
    int succ = successors[i];
    if (!visited[succ]) {
      RPOHelper(succ, bb_succ, output, visited);
    }
  }

  output.push(curr);
}

RPOLabelResult RPOLabel(const std::vector<std::vector<int>>& bb_succ) {
  std::stack<int> output;
  std::vector<bool> visited(bb_succ.size(), false);
  RPOHelper(0, bb_succ, output, visited);

  RPOLabelResult result;
  result.label_per_block = std::vector<int>(bb_succ.size(), -1);
  result.order.reserve(output.size());
  int i = 0;
  while (!output.empty()) {
    int curr = output.top();
    output.pop();

    result.order.push_back(curr);
    result.label_per_block[curr] = i++;
  }

  return result;
}

}  // namespace kush::khir