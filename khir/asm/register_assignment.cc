#include "khir/asm/register_assignment.h"

namespace kush::khir {

RegisterAssignment::RegisterAssignment(int reg, bool coalesced)
    : register_(reg) {}

void RegisterAssignment::SetRegister(int r) { register_ = r; }

int RegisterAssignment::Register() const { return register_; }

bool RegisterAssignment::IsRegister() const { return register_ >= 0; }

}  // namespace kush::khir