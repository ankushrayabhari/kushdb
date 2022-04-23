#pragma once

#include <vector>

#include "khir/asm/dfs_label.h"
#include "khir/asm/register_assignment.h"
#include "khir/program.h"

namespace kush::khir {

std::vector<RegisterAssignment> LinearScanRegisterAlloc(
    const Function& func, const std::vector<bool>& materialize_gep,
    const TypeManager& manager);

}  // namespace kush::khir