#pragma once

#include <vector>

#include "khir/asm/register_assignment.h"
#include "khir/asm/rpo_label.h"
#include "khir/program_builder.h"

namespace kush::khir {

std::vector<RegisterAssignment> LinearScanRegisterAlloc(
    const Function& func, const TypeManager& manager,
    const RPOLabelResult& rpo);

}  // namespace kush::khir