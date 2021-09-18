#pragma once

#include <vector>

namespace kush::khir {

struct LabelResult {
  std::vector<int> postorder_label;
  std::vector<int> preorder_label;
};

LabelResult DFSLabel(const std::vector<std::vector<int>>& succ);

}  // namespace kush::khir