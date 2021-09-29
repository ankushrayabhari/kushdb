#pragma once

#include <vector>

namespace kush::khir {

std::vector<std::vector<int>> ComputeDominators(
    const std::vector<std::vector<int>>& bb_pred);

}  // namespace kush::khir