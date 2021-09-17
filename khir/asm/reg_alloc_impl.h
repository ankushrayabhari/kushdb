#pragma once

namespace kush::khir {

enum class RegAllocImpl { STACK_SPILL, LINEAR_SCAN };

RegAllocImpl GetRegAllocImpl();

}  // namespace kush::khir