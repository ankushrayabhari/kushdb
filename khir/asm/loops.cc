#include "khir/asm/loops.h"

#include <iostream>
#include <queue>
#include <vector>

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_set.h"

#include "khir/asm/dominators.h"
#include "util/union_find.h"

namespace kush::khir {

bool DominatesHelper(const std::vector<std::vector<int>>& dom_tree, int curr,
                     int target) {
  if (curr == target) {
    return true;
  }

  for (int x : dom_tree[curr]) {
    if (DominatesHelper(dom_tree, x, target)) {
      return true;
    }
  }

  return false;
}

bool Dominates(const std::vector<std::vector<int>>& dom_tree, int i, int j) {
  return DominatesHelper(dom_tree, i, j);
}

void ConstructLoopTreeHelper(int header,
                             std::vector<std::vector<int>>& loop_tree,
                             const std::vector<absl::flat_hash_set<int>>& loops,
                             std::vector<int>& union_find) {
  absl::btree_set<int> loop_tree_children;
  for (int x : loops[header]) {
    loop_tree_children.insert(util::UnionFind::Find(union_find, x));
  }
  loop_tree_children.erase(header);
  loop_tree[header] =
      std::vector<int>(loop_tree_children.begin(), loop_tree_children.end());

  for (int x : loop_tree_children) {
    util::UnionFind::Union(union_find, x, header);
  }
}

std::vector<std::vector<int>> FindLoops(const std::vector<BasicBlock>& bb) {
  auto dom_tree = ComputeDominatorTree(bb);

  std::vector<std::vector<int>> backedges(bb.size());
  for (int i = 0; i < bb.size(); i++) {
    for (int j : bb[i].Successors()) {
      if (Dominates(dom_tree, j, i)) {
        backedges[j].push_back(i);
      }
    }
  }

  std::vector<absl::flat_hash_set<int>> loops(bb.size());
  for (int i = 0; i < bb.size(); i++) {
    if (backedges[i].empty()) {
      continue;
    }

    auto& loop = loops[i];
    loop.insert(i);

    // reverse BFS from each of the backedge nodes ignoring any edge to i
    std::vector<bool> visited(bb.size(), false);
    visited[i] = true;

    std::queue<int> bfs;
    for (int z : backedges[i]) {
      bfs.push(z);
      visited[z] = true;
    }

    while (!bfs.empty()) {
      int z = bfs.front();
      bfs.pop();
      loop.insert(z);

      for (int y : bb[z].Predecessors()) {
        if (!visited[y]) {
          visited[y] = true;

          if (Dominates(dom_tree, i, y)) {
            bfs.push(y);
          }
        }
      }
    }
  }

  // construct loop tree
  std::vector<std::vector<int>> loop_tree(bb.size());
  std::vector<int> union_find(bb.size());
  for (int i = 0; i < union_find.size(); i++) {
    union_find[i] = i;
  }

  std::vector<int> order;
  for (int i = 0; i < loops.size(); i++) {
    if (!loops[i].empty()) {
      order.push_back(i);
    }
  }
  std::sort(order.begin(), order.end(), [&](int a, int b) -> bool {
    return loops[a].size() < loops[b].size();
  });

  for (int i : order) {
    ConstructLoopTreeHelper(i, loop_tree, loops, union_find);
  }
  return loop_tree;
}

}  // namespace kush::khir