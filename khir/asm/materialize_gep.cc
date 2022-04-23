#include <vector>

#include "khir/instruction.h"
#include "khir/program.h"

namespace kush::khir {

void MarkMaterialization(khir::Value value, const std::vector<uint64_t>& instrs,
                         std::vector<bool>& should_materialize) {
  if (!value.IsConstantGlobal() &&
      OpcodeFrom(GenericInstructionReader(instrs[value.GetIdx()]).Opcode()) ==
          Opcode::GEP_STATIC) {
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
          case Opcode::PTR_CAST:
          case Opcode::CALL_ARG: {
            khir::Value value(Type3InstructionReader(instrs[i]).Arg());
            MarkMaterialization(value, instrs, should_materialize);
            break;
          }

          case Opcode::PTR_STORE:
          case Opcode::PHI_MEMBER: {
            khir::Value value(Type2InstructionReader(instrs[i]).Arg1());
            MarkMaterialization(value, instrs, should_materialize);
            break;
          }

          case Opcode::PTR_CMP_NULLPTR: {
            khir::Value value(Type2InstructionReader(instrs[i]).Arg0());
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