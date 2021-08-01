#include "khir/asm/register_assignment.h"

namespace kush::khir {

RegisterAssignment::RegisterAssignment() : register_(-1) {}

RegisterAssignment::RegisterAssignment(int reg) : register_(reg) {}

void RegisterAssignment::SetRegister(int r) { register_ = r; }

void RegisterAssignment::Spill() { register_ = -1; }

int RegisterAssignment::Register() const { return register_; }

bool RegisterAssignment::IsRegister() const { return register_ >= 0; }

}  // namespace kush::khir