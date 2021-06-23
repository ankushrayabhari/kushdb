#pragma once

#include <vector>

namespace kush::util::UnionFind {

int Find(std::vector<int>& parent, int i);

void Union(std::vector<int>& parent, int x, int y);

}  // namespace kush::util::UnionFind