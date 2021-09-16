#pragma once

#include <vector>

namespace kush::khir {

struct RPOLabelResult {
  std::vector<int> label_per_block;
  std::vector<int> order;
};

RPOLabelResult RPOLabel(const std::vector<std::vector<int>>& bb_succ);

}  // namespace kush::khir