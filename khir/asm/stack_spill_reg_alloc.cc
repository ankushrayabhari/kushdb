#include "khir/asm/stack_spill_reg_alloc.h"

#include <vector>

#include "khir/asm/register_assignment.h"
#include "khir/instruction.h"
#include "khir/opcode.h"

namespace kush::khir {

/*
Available for allocation:
  RBX  = 0
  RCX  = 1
  RDX  = 2
  RSI  = 3
  RDI  = 4
  R8   = 5
  R9   = 6
  R10  = 7
  R11  = 8
  R12  = 9
  R13  = 10
  R14  = 11
  R15  = 12

  XMM0 = 50
  XMM1 = 51
  XMM2 = 52
  XMM3 = 53
  XMM4 = 54
  XMM5 = 55
  XMM6 = 56

  FLAG = 100

Reserved/Scratch
  RSP, RBP, RAX, XMM7
*/

std::vector<RegisterAssignment> StackSpillingRegisterAlloc(
    const std::vector<uint64_t>& instrs) {
  std::vector<RegisterAssignment> assignments(instrs.size(),
                                              RegisterAssignment(-1, false));

  for (int i = 0; i < instrs.size(); i++) {
    auto opcode = OpcodeFrom(GenericInstructionReader(instrs[i]).Opcode());
    switch (opcode) {
      case Opcode::I8_STORE:
      case Opcode::I16_STORE:
      case Opcode::I32_STORE:
      case Opcode::I64_STORE:
      case Opcode::PTR_STORE:
        assignments[i].SetRegister(12);
        break;

      default:
        break;
    }
  }

  return assignments;
}

}  // namespace kush::khir