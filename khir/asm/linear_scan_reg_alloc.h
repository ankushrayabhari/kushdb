#pragma once

#include <vector>

#include "khir/asm/live_intervals.h"
#include "khir/asm/register_assignment.h"

namespace kush::khir {

std::vector<RegisterAssignment> LinearScanRegisterAlloc(
    std::vector<LiveInterval>& live_intervals,
    const std::vector<uint64_t>& instrs, const TypeManager& manager);

}  // namespace kush::khir