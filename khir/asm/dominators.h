#pragma once

#include <vector>

#include "khir/program.h"

namespace kush::khir {

std::vector<std::vector<int>> ComputeDominatorTree(
    const std::vector<BasicBlock>& bb);

}  // namespace kush::khir