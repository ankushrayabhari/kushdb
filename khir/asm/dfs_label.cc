#include "khir/asm/dfs_label.h"

#include <stack>
#include <vector>

namespace kush::khir {

void DFS(int curr, const std::vector<std::vector<int>>& bb_succ,
         std::vector<int>& preorder, std::vector<int>& postorder,
         std::vector<bool>& visited, int& idx) {
  preorder[curr] = idx++;
  visited[curr] = true;

  const auto& successors = bb_succ[curr];
  for (int succ : successors) {
    if (!visited[succ]) {
      DFS(succ, bb_succ, preorder, postorder, visited, idx);
    }
  }

  postorder[curr] = idx++;
}

LabelResult DFSLabel(const std::vector<std::vector<int>>& bb_succ) {
  LabelResult result;
  std::vector<bool> visited(bb_succ.size(), false);
  int idx = 0;
  result.preorder_label = std::vector<int>(bb_succ.size(), -1);
  result.postorder_label = std::vector<int>(bb_succ.size(), -1);
  DFS(0, bb_succ, result.preorder_label, result.postorder_label, visited, idx);
  return result;
}

}  // namespace kush::khir