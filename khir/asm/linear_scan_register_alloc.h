#pragma once

#include <optional>
#include <vector>

#include "khir/asm/live_intervals.h"
#include "khir/asm/register.h"

namespace kush::khir {

std::vector<int> AssignRegisters(
    const std::vector<LiveInterval>& live_intervals);

}  // namespace kush::khir