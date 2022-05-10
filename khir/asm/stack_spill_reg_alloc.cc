#include "khir/asm/stack_spill_reg_alloc.h"

#include <vector>

#include "khir/asm/register.h"
#include "khir/asm/register_assignment.h"
#include "khir/instruction.h"
#include "khir/opcode.h"

namespace kush::khir {

std::vector<RegisterAssignment> StackSpillingRegisterAlloc(
    const std::vector<uint64_t>& instrs) {
  std::vector<RegisterAssignment> assignments(instrs.size());

  for (int i = 0; i < instrs.size(); i++) {
    auto opcode = OpcodeFrom(GenericInstructionReader(instrs[i]).Opcode());
    switch (opcode) {
      case Opcode::I8_STORE:
      case Opcode::I16_STORE:
      case Opcode::I32_STORE:
      case Opcode::I64_STORE:
      case Opcode::PTR_STORE:
        assignments[i].SetRegister(GPRegister::R15.Id());
        break;

      default:
        break;
    }
  }

  return assignments;
}

}  // namespace kush::khir