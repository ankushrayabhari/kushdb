#include "khir/asm/reg_alloc_impl.h"

#include "absl/flags/flag.h"

ABSL_FLAG(std::string, reg_alloc, "linear_scan",
          "Register Allocation: stack_spill or linear_scan");

namespace kush::khir {

RegAllocImpl GetRegAllocImpl() {
  if (FLAGS_reg_alloc.CurrentValue() == "stack_spill") {
    return RegAllocImpl::STACK_SPILL;
  } else if (FLAGS_reg_alloc.CurrentValue() == "linear_scan") {
    return RegAllocImpl::LINEAR_SCAN;
  } else {
    throw std::runtime_error("Unknown reg alloc impl.");
  }
}

}  // namespace kush::khir