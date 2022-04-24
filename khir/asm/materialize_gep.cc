#include <vector>

#include "khir/instruction.h"
#include "khir/program.h"

namespace kush::khir {

void MarkMaterialization(Value value, const std::vector<uint64_t>& instrs,
                         std::vector<bool>& should_materialize) {
  auto opcode =
      OpcodeFrom(GenericInstructionReader(instrs[value.GetIdx()]).Opcode());
  if (!value.IsConstantGlobal() && (opcode == Opcode::GEP_STATIC)) {
    should_materialize[value.GetIdx()] = true;
  }
}

std::vector<bool> ComputeGEPMaterialize(const Function& func) {
  const auto& instrs = func.Instrs();
  std::vector<bool> should_materialize(instrs.size(), false);

  const auto& basic_blocks = func.BasicBlocks();
  for (const auto& bb : basic_blocks) {
    for (const auto& [seg_start, seg_end] : bb.Segments()) {
      for (int i = seg_start; i <= seg_end; i++) {
        auto opcode = OpcodeFrom(GenericInstructionReader(instrs[i]).Opcode());
        switch (opcode) {
          case Opcode::RETURN_VALUE:
          case Opcode::CALL_INDIRECT:
          case Opcode::PTR_CAST:
          case Opcode::CALL_ARG: {
            Value value(Type3InstructionReader(instrs[i]).Arg());
            MarkMaterialization(value, instrs, should_materialize);
            break;
          }

          case Opcode::PTR_STORE:
          case Opcode::PHI_MEMBER: {
            Value value(Type2InstructionReader(instrs[i]).Arg1());
            MarkMaterialization(value, instrs, should_materialize);
            break;
          }

          case Opcode::GEP_STATIC_OFFSET:
          case Opcode::PTR_CMP_NULLPTR: {
            Value value(Type2InstructionReader(instrs[i]).Arg0());
            MarkMaterialization(value, instrs, should_materialize);
            break;
          }

          default:
            break;
        }
      }
    }
  }

  return should_materialize;
}

}  // namespace kush::khir