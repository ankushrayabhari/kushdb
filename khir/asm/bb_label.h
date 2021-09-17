#pragma once

#include <vector>

namespace kush::khir {

struct BBLabelResult {
  std::vector<int> postorder_label;
  std::vector<int> preorder_label;
};

BBLabelResult BBLabel(const std::vector<std::vector<int>>& bb_succ);

}  // namespace kush::khir