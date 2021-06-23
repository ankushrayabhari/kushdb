#include "util/union_find.h"

#include <vector>

namespace kush::util::UnionFind {

// TODO: replace with better union find impl
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

}  // namespace kush::util::UnionFind
