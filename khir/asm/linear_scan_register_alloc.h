#pragma once

#include <optional>
#include <vector>

#include "khir/asm/live_intervals.h"
#include "khir/asm/register.h"

namespace kush::khir {

std::vector<int> AssignRegisters(std::vector<LiveInterval>& live_intervals,
                                 const std::vector<uint64_t>& instrs,
                                 const TypeManager& manager);

}  // namespace kush::khir