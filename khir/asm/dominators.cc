#include "khir/asm/dominators.h"

#include <iostream>
#include <queue>
#include <vector>

#include "absl/container/btree_set.h"

namespace kush::khir {

std::vector<std::vector<int>> ComputeDominators(
    const std::vector<std::vector<int>>& bb_pred) {
  // x in dom_parent[y] iff x strictly dominates y
  std::vector<absl::btree_set<int>> dom_parent(bb_pred.size());
  for (int i = 0; i < bb_pred.size(); i++) {
    for (int j = 0; j < bb_pred.size(); j++) {
      dom_parent[i].insert(j);
    }
  }

  bool changes = true;
  while (changes) {
    changes = false;

    for (int i = 0; i < bb_pred.size(); i++) {
      auto copy = dom_parent[i];

      dom_parent[i].clear();
      const auto& pred = bb_pred[i];
      if (pred.size() > 0) {
        dom_parent[i] = absl::btree_set<int>(dom_parent[pred[0]].begin(),
                                             dom_parent[pred[0]].end());
        for (auto it = dom_parent[i].begin(); it != dom_parent[i].end();) {
          bool found = true;
          for (int k = 1; k < pred.size(); k++) {
            if (!dom_parent[pred[k]].contains((*it))) {
              found = false;
              break;
            }
          }
          if (found) {
            it++;
          } else {
            it = dom_parent[i].erase(it);
          }
        }
      }
      dom_parent[i].insert(i);

      changes = changes || copy != dom_parent[i];
    }
  }

  for (int i = 0; i < dom_parent.size(); i++) {
    dom_parent[i].erase(i);
  }

  // y in dom_children[x] iff x strictly dominates y
  std::vector<absl::btree_set<int>> dom_children(bb_pred.size());
  for (int node = 0; node < dom_parent.size(); node++) {
    for (int dom : dom_parent[node]) {
      dom_children[dom].insert(node);
    }
  }

  std::vector<std::vector<int>> dom_tree(bb_pred.size());
  std::queue<int> worklist;
  for (int i = 0; i < dom_parent.size(); i++) {
    if (dom_parent[i].empty()) {
      worklist.push(i);
    }
  }

  while (!worklist.empty()) {
    int curr = worklist.front();
    worklist.pop();

    for (int succ : dom_children[curr]) {
      dom_parent[succ].erase(curr);
      // erase edge between curr -> succ
      if (dom_parent[succ].empty()) {
        dom_tree[curr].push_back(succ);
        worklist.push(succ);
      }
    }
    dom_children[curr].clear();
  }

  return dom_tree;
}

}  // namespace kush::khir