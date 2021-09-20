#pragma once

#include <vector>

#include "khir/asm/dfs_label.h"
#include "khir/asm/register_assignment.h"
#include "khir/program_builder.h"

namespace kush::khir {

std::vector<RegisterAssignment> LinearScanRegisterAlloc(
    const Function& func, const TypeManager& manager);

}  // namespace kush::khir