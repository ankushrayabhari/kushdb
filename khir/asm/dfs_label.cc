#include "khir/asm/dfs_label.h"

#include <stack>
#include <vector>

namespace kush::khir {

void DFS(int curr, const std::vector<BasicBlock>& bb,
         std::vector<int>& preorder, std::vector<int>& postorder,
         std::vector<bool>& visited, int& idx) {
  preorder[curr] = idx++;
  visited[curr] = true;

  for (int succ : bb[curr].Successors()) {
    if (!visited[succ]) {
      DFS(succ, bb, preorder, postorder, visited, idx);
    }
  }

  postorder[curr] = idx++;
}

LabelResult DFSLabel(const std::vector<BasicBlock>& bb) {
  LabelResult result;
  std::vector<bool> visited(bb.size(), false);
  int idx = 0;
  result.preorder_label = std::vector<int>(bb.size(), -1);
  result.postorder_label = std::vector<int>(bb.size(), -1);
  DFS(0, bb, result.preorder_label, result.postorder_label, visited, idx);
  return result;
}

}  // namespace kush::khir