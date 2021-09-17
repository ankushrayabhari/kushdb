#pragma once

#include <vector>

#include "khir/asm/bb_label.h"
#include "khir/asm/register_assignment.h"
#include "khir/program_builder.h"

namespace kush::khir {

std::vector<RegisterAssignment> LinearScanRegisterAlloc(
    const Function& func, const TypeManager& manager, const BBLabelResult& rpo);

}  // namespace kush::khir