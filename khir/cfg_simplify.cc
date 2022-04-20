#include "khir/cfg_simplify.h"

#include <iostream>
#include <queue>
#include <vector>

#include "khir/instruction.h"
#include "khir/program.h"

namespace kush::khir {

std::vector<BasicBlock> CFGSimplify(std::vector<uint64_t>& instrs,
                                    std::vector<BasicBlock> basic_blocks) {
  // Step 1: Remove any unreachable blocks
  std::vector<bool> deleted(basic_blocks.size(), false);
  {
    std::queue<int> worklist;
    // start at 1 since basic_blocks[0] is always the entry block
    for (int i = 1; i < basic_blocks.size(); i++) {
      if (basic_blocks[i].Predecessors().empty()) {
        worklist.push(i);
        deleted[i] = true;
      }
    }

    while (!worklist.empty()) {
      auto curr = worklist.front();
      worklist.pop();

      for (auto succ : basic_blocks[curr].Successors()) {
        if (deleted[succ]) continue;

        bool all_deleted = true;
        for (auto pred : basic_blocks[succ].Predecessors()) {
          if (!deleted[pred]) {
            all_deleted = false;
            break;
          }
        }

        if (all_deleted) {
          worklist.push(succ);
          deleted[succ] = true;
        }
      }
    }
  }

  absl::flat_hash_map<int, int> bb_idx;
  int new_idx = 0;
  for (int i = 0; i < basic_blocks.size(); i++) {
    if (deleted[i]) continue;
    bb_idx[i] = new_idx++;
  }

  std::vector<BasicBlock> output;
  for (int i = 0; i < basic_blocks.size(); i++) {
    if (deleted[i]) continue;

    std::vector<int> predecessors;
    for (int pred : basic_blocks[i].Predecessors()) {
      if (deleted[pred]) continue;
      predecessors.push_back(bb_idx.at(pred));
    }
    basic_blocks[i].SetPredecessors(std::move(predecessors));

    std::vector<int> successors;
    for (int succ : basic_blocks[i].Successors()) {
      if (deleted[succ]) continue;
      successors.push_back(bb_idx.at(succ));
    }
    basic_blocks[i].SetSuccessors(std::move(successors));

    if (basic_blocks[i].Segments().size() != 1) {
      throw std::runtime_error("Expected only single segment bbs.");
    }

    // rewrite the branch instruction
    auto end_instr_idx = basic_blocks[i].Segments()[0].second;

    auto reader = Type5InstructionReader(instrs[end_instr_idx]);
    switch (OpcodeFrom(reader.Opcode())) {
      case Opcode::BR: {
        auto next_bb = reader.Marg0();
        instrs[end_instr_idx] = Type5InstructionBuilder()
                                    .SetOpcode(OpcodeTo(Opcode::BR))
                                    .SetMarg0(bb_idx.at(next_bb))
                                    .Build();
        break;
      }

      case Opcode::CONDBR: {
        auto arg = reader.Arg();
        auto next_bb1 = reader.Marg0();
        auto next_bb2 = reader.Marg1();
        instrs[end_instr_idx] = Type5InstructionBuilder()
                                    .SetOpcode(OpcodeTo(Opcode::CONDBR))
                                    .SetArg(arg)
                                    .SetMarg0(bb_idx.at(next_bb1))
                                    .SetMarg1(bb_idx.at(next_bb2))
                                    .Build();
        break;
      }

      case Opcode::RETURN:
      case Opcode::RETURN_VALUE:
        break;

      default:
        throw std::runtime_error("BB not terminated");
    }

    output.push_back(std::move(basic_blocks[i]));
  }

  return output;
}

}  // namespace kush::khir