#include "khir/asm/live_intervals.h"

#include <iostream>
#include <stack>
#include <vector>

#include "absl/container/flat_hash_set.h"

#include "khir/program_builder.h"

namespace kush::khir {

LiveInterval::LiveInterval(int start, int end) : start_(start), end_(end) {}

int LiveInterval::Start() const { return start_; }

int LiveInterval::End() const { return end_; }

int Find(std::vector<int>& parent, int i) {
  if (parent[i] == i) return i;
  parent[i] = Find(parent, parent[i]);
  return parent[i];
}

void Union(std::vector<int>& parent, int x, int y) {
  int xset = Find(parent, x);
  int yset = Find(parent, y);

  if (xset == yset) {
    return;
  }

  parent[xset] = yset;
}

std::vector<std::vector<int>> ComputeBackedges(const Function& func) {
  // TODO: replace with linear time dominator algorithm
  const auto& bb = func.BasicBlocks();
  const auto& bb_succ = func.BasicBlockSuccessors();
  const auto& bb_pred = func.BasicBlockPredecessors();
  std::vector<absl::flat_hash_set<int>> dom(bb.size());
  dom[0].insert(0);
  for (int i = 1; i < bb.size(); i++) {
    for (int j = 0; j < bb.size(); j++) {
      dom[i].insert(j);
    }
  }

  bool changes = true;
  while (changes) {
    changes = false;

    for (int i = 1; i < bb.size(); i++) {
      int initial_size = dom[i].size();

      dom[i].clear();
      const auto& pred = bb_pred[i];
      if (pred.size() > 0) {
        dom[i] =
            absl::flat_hash_set<int>(dom[pred[0]].begin(), dom[pred[0]].end());
        for (int k = 1; k < pred.size(); k++) {
          for (int x : dom[pred[k]]) {
            if (!dom[i].contains(x)) {
              dom[i].erase(x);
            }
          }
        }
      }
      dom[i].insert(i);

      changes = changes || dom[i].size() != initial_size;
    }
  }

  std::vector<std::vector<int>> backedges(bb.size());
  for (int i = 0; i < bb.size(); i++) {
    for (int x : bb_succ[i]) {
      if (dom[i].contains(x)) {
        backedges[x].push_back(i);
      }
    }
  }

  return backedges;
}

void Collapse(std::vector<int>& loop_parent, std::vector<int>& union_find,
              const absl::flat_hash_set<int>& loop_body, int loop_header) {
  for (int z : loop_body) {
    loop_parent[z] = loop_header;
    Union(union_find, z, loop_header);
  }
}

void FindLoopHelper(int curr, const std::vector<std::vector<int>>& bb_succ,
                    const std::vector<std::vector<int>>& bb_pred,
                    const std::vector<std::vector<int>>& backedges,
                    std::vector<bool>& visited, std::vector<int>& union_find,
                    std::vector<int>& loop_parent) {
  // DFS
  for (auto succ : bb_succ[curr]) {
    if (!visited[succ]) {
      visited[succ] = true;
      FindLoopHelper(succ, bb_succ, bb_pred, backedges, visited, union_find,
                     loop_parent);
    }
  }

  absl::flat_hash_set<int> loop_body;
  absl::flat_hash_set<int> work_list;
  for (int y : backedges[curr]) {
    work_list.insert(Find(union_find, y));
  }
  work_list.erase(curr);

  while (!work_list.empty()) {
    int y = *work_list.begin();
    work_list.erase(y);

    loop_body.insert(y);

    for (int z : bb_pred[y]) {
      if (std::find(backedges[y].begin(), backedges[y].end(), z) ==
          backedges[y].end()) {
        auto repr = Find(union_find, z);
        if (repr != curr && !loop_body.contains(repr) &&
            !work_list.contains(repr)) {
          work_list.insert(repr);
        }
      }
    }
  }

  if (!loop_body.empty()) {
    Collapse(loop_parent, union_find, loop_body, curr);
  }
}

std::vector<int> FindLoops(const Function& func) {
  const auto& bb = func.BasicBlocks();
  const auto& bb_succ = func.BasicBlockSuccessors();
  const auto& bb_pred = func.BasicBlockPredecessors();
  auto backedges = ComputeBackedges(func);

  // Find all loops
  std::vector<int> union_find(bb.size());
  for (int i = 0; i < bb.size(); i++) {
    union_find[i] = i;
  }

  std::vector<int> loop_parent(bb.size(), -1);
  std::vector<bool> visited(bb.size(), false);
  FindLoopHelper(0, bb_succ, bb_pred, backedges, visited, union_find,
                 loop_parent);
  return loop_parent;
}

std::vector<LiveInterval> ComputeLiveIntervals(const Function& func) {
  const auto& bb = func.BasicBlocks();
  const auto& bb_succ = func.BasicBlockSuccessors();

  std::cerr << "digraph {\n";
  for (int i = 0; i < bb.size(); i++) {
    for (int j : bb_succ[i]) {
      std::cerr << "  " << i << " -> " << j << "\n";
    }
  }
  std::cerr << "}\n";

  auto loop_parent = FindLoops(func);
  for (int i = 0; i < bb.size(); i++) {
    std::cerr << i << ":  " << loop_parent[i] << std::endl;
  }

  return {};
}

}  // namespace kush::khir