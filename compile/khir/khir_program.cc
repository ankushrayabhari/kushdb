#include "compile/khir/khir_program.h"

#include <cstdint>
#include <stdexcept>
#include <vector>

#include "absl/types/span.h"

#include "type_safe/strong_typedef.hpp"

#include "compile/khir/instruction.h"
#include "compile/khir/opcode.h"

namespace kush::khir {

// I1
Value KHIRProgram::LNotI1(Value v) {
  uint32_t value = instructions_.size();
  instructions_.push_back(Type2InstructionBuilder()
                              .SetOpcode(Opcode::I1_LNOT)
                              .SetArg0(static_cast<uint32_t>(v))
                              .Build());
  return static_cast<Value>(value);
}

Value KHIRProgram::CmpI1(CompType cmp, Value v1, Value v2) {
  Opcode opcode;
  switch (cmp) {
    case CompType::EQ:
      opcode = Opcode::I1_CMP_EQ;
      break;

    case CompType::NE:
      opcode = Opcode::I1_CMP_NE;
      break;

    default:
      throw std::runtime_error("Invalid comp type for I1");
      break;
  }

  uint32_t value = instructions_.size();
  instructions_.push_back(Type2InstructionBuilder()
                              .SetOpcode(opcode)
                              .SetArg0(static_cast<uint32_t>(v1))
                              .SetArg0(static_cast<uint32_t>(v2))
                              .Build());
  return static_cast<Value>(value);
}

Value KHIRProgram::ConstI1(bool v) {
  uint32_t value = instructions_.size();
  instructions_.push_back(Type1InstructionBuilder()
                              .SetOpcode(Opcode::I1_CONST)
                              .SetConstant(v ? 1 : 0)
                              .Build());
  return static_cast<Value>(value);
}

}  // namespace kush::khir