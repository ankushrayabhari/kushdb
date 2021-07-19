#pragma once

#include <cstdint>
#include <vector>

#include "khir/asm/register_assignment.h"

namespace kush::khir {

std::vector<RegisterAssignment> StackSpillingRegisterAlloc(
    const std::vector<uint64_t>& instrs);

}  // namespace kush::khir