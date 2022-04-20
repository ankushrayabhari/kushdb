#include "khir/asm/dominators.h"

#include <algorithm>
#include <iostream>
#include <queue>
#include <vector>

#include "absl/container/btree_set.h"

namespace kush::khir {

void POHelper(int curr, const std::vector<BasicBlock>& bb,
              std::vector<bool>& visited, std::vector<int>& result) {
  if (visited[curr]) {
    return;
  }

  visited[curr] = true;

  for (auto s : bb[curr].Successors()) {
    POHelper(s, bb, visited, result);
  }

  result.push_back(curr);
}

std::vector<int> GeneratePO(const std::vector<BasicBlock>& bb) {
  std::vector<bool> visited(bb.size(), false);
  std::vector<int> result;
  POHelper(0, bb, visited, result);
  return result;
}

std::vector<std::vector<int>> ComputeDominatorTree(
    const std::vector<BasicBlock>& bb) {
  int num_blocks = bb.size();
  auto po = GeneratePO(bb);
  std::vector<int> label(num_blocks, -1);
  for (int i = 0; i < po.size(); i++) {
    label[po[i]] = i;
  }
  auto rpo = po;
  std::reverse(rpo.begin(), rpo.end());

  std::vector<int> idom(num_blocks, -1);
  idom[0] = 0;
  bool changed = true;

  while (changed) {
    changed = false;

    for (int b : rpo) {
      if (b == 0) {
        continue;
      }

      int new_idom = -1;
      for (auto p : bb[b].Predecessors()) {
        if (idom[p] < 0) {
          continue;
        }

        if (new_idom < 0) {
          new_idom = p;
        } else {
          // intersect p with new_idom

          int finger1 = p;
          int finger2 = new_idom;
          while (finger1 != finger2) {
            while (label[finger1] < label[finger2]) {
              finger1 = idom[finger1];
            }

            while (label[finger2] < label[finger1]) {
              finger2 = idom[finger2];
            }
          }

          new_idom = finger1;
        }
      }

      if (new_idom < 0) {
        throw std::runtime_error("New idom not found.");
      }

      if (idom[b] != new_idom) {
        idom[b] = new_idom;
        changed = true;
      }
    }
  }

  std::vector<std::vector<int>> dom_tree(num_blocks);
  for (int i = 1; i < num_blocks; i++) {
    if (idom[i] >= 0) {
      dom_tree[idom[i]].push_back(i);
    }
  }

  return dom_tree;
}

}  // namespace kush::khir