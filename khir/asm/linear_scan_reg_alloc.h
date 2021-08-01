#pragma once

#include <vector>

#include "khir/asm/register_assignment.h"
#include "khir/program_builder.h"

namespace kush::khir {

struct RegisterAllocationResult {
  std::vector<RegisterAssignment> assignment;
  std::vector<int> order;
};

RegisterAllocationResult LinearScanRegisterAlloc(const Function& func,
                                                 const TypeManager& manager);

}  // namespace kush::khir