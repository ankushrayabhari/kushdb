#pragma once

#include <vector>

#include "khir/asm/register_assignment.h"
#include "khir/program_builder.h"

namespace kush::khir {

std::pair<std::vector<RegisterAssignment>, std::vector<int>> LinearScanRegisterAlloc(
    const Function& func, const TypeManager& manager);

}  // namespace kush::khir