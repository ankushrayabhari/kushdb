#pragma once

#include <vector>

namespace kush::khir {

std::vector<std::vector<int>> FindLoops(
    const std::vector<std::vector<int>>& bb_succ,
    const std::vector<std::vector<int>>& bb_pred);

}  // namespace kush::khir