#pragma once

#include <vector>

#include "khir/program.h"

namespace kush::khir {

struct LabelResult {
  std::vector<int> postorder_label;
  std::vector<int> preorder_label;
};

LabelResult DFSLabel(const std::vector<BasicBlock>& bb);

}  // namespace kush::khir