#include "khir/cfg_simplify.h"

#include <iostream>
#include <queue>
#include <vector>

#include "khir/instruction.h"
#include "khir/program.h"

namespace kush::khir {

std::vector<bool> GetDeletedBlocks(
    const std::vector<BasicBlock>& basic_blocks) {
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
  return deleted;
}

void RewriteInstr(uint64_t& instr,
                  const absl::flat_hash_map<uint32_t, uint32_t>& value_map) {
  auto opcode = OpcodeFrom(GenericInstructionReader(instr).Opcode());

  switch (opcode) {
    case Opcode::I1_AND:
    case Opcode::I1_OR:
    case Opcode::I1_CMP_EQ:
    case Opcode::I1_CMP_NE:
    case Opcode::I8_ADD:
    case Opcode::I8_MUL:
    case Opcode::I8_SUB:
    case Opcode::I8_CMP_EQ:
    case Opcode::I8_CMP_NE:
    case Opcode::I8_CMP_LT:
    case Opcode::I8_CMP_LE:
    case Opcode::I8_CMP_GT:
    case Opcode::I8_CMP_GE:
    case Opcode::I16_ADD:
    case Opcode::I16_MUL:
    case Opcode::I16_SUB:
    case Opcode::I16_CMP_EQ:
    case Opcode::I16_CMP_NE:
    case Opcode::I16_CMP_LT:
    case Opcode::I16_CMP_LE:
    case Opcode::I16_CMP_GT:
    case Opcode::I16_CMP_GE:
    case Opcode::I32_ADD:
    case Opcode::I32_MUL:
    case Opcode::I32_SUB:
    case Opcode::I32_CMP_EQ:
    case Opcode::I32_CMP_NE:
    case Opcode::I32_CMP_LT:
    case Opcode::I32_CMP_LE:
    case Opcode::I32_CMP_GT:
    case Opcode::I32_CMP_GE:
    case Opcode::I64_ADD:
    case Opcode::I64_MUL:
    case Opcode::I64_SUB:
    case Opcode::I64_LSHIFT:
    case Opcode::I64_RSHIFT:
    case Opcode::I64_AND:
    case Opcode::I64_OR:
    case Opcode::I64_XOR:
    case Opcode::I64_CMP_EQ:
    case Opcode::I64_CMP_NE:
    case Opcode::I64_CMP_LT:
    case Opcode::I64_CMP_LE:
    case Opcode::I64_CMP_GT:
    case Opcode::I64_CMP_GE:
    case Opcode::F64_ADD:
    case Opcode::F64_MUL:
    case Opcode::F64_SUB:
    case Opcode::F64_DIV:
    case Opcode::F64_CMP_EQ:
    case Opcode::F64_CMP_NE:
    case Opcode::F64_CMP_LT:
    case Opcode::F64_CMP_LE:
    case Opcode::F64_CMP_GT:
    case Opcode::F64_CMP_GE:
    case Opcode::PHI_MEMBER:
    case Opcode::I1_LNOT:
    case Opcode::I1_ZEXT_I8:
    case Opcode::I1_ZEXT_I64:
    case Opcode::I8_ZEXT_I64:
    case Opcode::I8_CONV_F64:
    case Opcode::I16_ZEXT_I64:
    case Opcode::I16_CONV_F64:
    case Opcode::I32_ZEXT_I64:
    case Opcode::I32_CONV_F64:
    case Opcode::I64_CONV_F64:
    case Opcode::I64_TRUNC_I16:
    case Opcode::I64_TRUNC_I32:
    case Opcode::F64_CONV_I64:
    case Opcode::PTR_CMP_NULLPTR:
    case Opcode::I1_LOAD:
    case Opcode::I8_LOAD:
    case Opcode::I16_LOAD:
    case Opcode::I32_LOAD:
    case Opcode::I32_VEC8_LOAD:
    case Opcode::I64_LOAD:
    case Opcode::F64_LOAD:
    case Opcode::I8_STORE:
    case Opcode::I16_STORE:
    case Opcode::I32_STORE:
    case Opcode::I64_STORE:
    case Opcode::F64_STORE:
    case Opcode::PTR_STORE:
    case Opcode::GEP_STATIC_OFFSET:
    case Opcode::GEP_DYNAMIC_OFFSET:
    case Opcode::I32_CMP_EQ_ANY_CONST_VEC4:
    case Opcode::I32_CMP_EQ_ANY_CONST_VEC8: {
      Type2InstructionReader reader(instr);
      auto v0 = reader.Arg0();
      auto v1 = reader.Arg1();

      while (value_map.contains(v0)) {
        v0 = value_map.at(v0);
      }

      while (value_map.contains(v1)) {
        v1 = value_map.at(v1);
      }

      instr = Type2InstructionBuilder(instr).SetArg0(v0).SetArg1(v1).Build();
      return;
    }

    case Opcode::GEP_DYNAMIC:
    case Opcode::PTR_CAST:
    case Opcode::PTR_LOAD:
    case Opcode::RETURN_VALUE:
    case Opcode::CALL_ARG: {
      Type3InstructionReader reader(instr);
      auto v0 = reader.Arg();

      while (value_map.contains(v0)) {
        v0 = value_map.at(v0);
      }

      instr = Type3InstructionBuilder(instr).SetArg(v0).Build();
      return;
    }

    case Opcode::CONDBR: {
      Type5InstructionReader reader(instr);
      auto v0 = reader.Arg();

      while (value_map.contains(v0)) {
        v0 = value_map.at(v0);
      }

      instr = Type5InstructionBuilder(instr).SetArg(v0).Build();
      return;
    }

    case Opcode::CALL_INDIRECT:
    case Opcode::CALL:
    case Opcode::RETURN:
    case Opcode::BR:
    case Opcode::FUNC_ARG:
    case Opcode::GEP_STATIC:
    case Opcode::PHI:
    case Opcode::ALLOCA:
      return;
  }
}

void RewriteBlock(BasicBlock& block, std::vector<uint64_t>& instrs,
                  const std::vector<bool>& deleted,
                  const absl::flat_hash_map<int, int>& bb_idx) {
  std::vector<int> predecessors;
  for (int pred : block.Predecessors()) {
    if (deleted[pred]) continue;
    predecessors.push_back(bb_idx.at(pred));
  }
  block.SetPredecessors(std::move(predecessors));

  std::vector<int> successors;
  for (int succ : block.Successors()) {
    if (deleted[succ]) continue;
    successors.push_back(bb_idx.at(succ));
  }
  block.SetSuccessors(std::move(successors));

  // rewrite the branch instruction
  auto end_instr_idx = block.Segments().back().second;

  auto reader = Type5InstructionReader(instrs[end_instr_idx]);
  switch (OpcodeFrom(reader.Opcode())) {
    case Opcode::BR: {
      auto next_bb = reader.Marg0();
      instrs[end_instr_idx] = Type5InstructionBuilder(instrs[end_instr_idx])
                                  .SetMarg0(bb_idx.at(next_bb))
                                  .Build();
      break;
    }

    case Opcode::CONDBR: {
      auto next_bb1 = reader.Marg0();
      auto next_bb2 = reader.Marg1();
      instrs[end_instr_idx] = Type5InstructionBuilder(instrs[end_instr_idx])
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
}

void MergeBasicBlocks(std::vector<uint64_t>& instrs,
                      std::vector<BasicBlock>& basic_blocks,
                      std::vector<bool>& deleted) {
  std::vector<std::vector<int>> succ(basic_blocks.size());
  std::vector<std::vector<int>> pred(basic_blocks.size());

  for (int i = 0; i < basic_blocks.size(); i++) {
    for (int x : basic_blocks[i].Successors()) {
      if (!deleted[x]) {
        succ[i].push_back(x);
      }
    }

    for (int x : basic_blocks[i].Predecessors()) {
      if (!deleted[x]) {
        pred[i].push_back(x);
      }
    }
  }

  absl::flat_hash_map<uint32_t, uint32_t> value_map;
  for (int i = 0; i < basic_blocks.size(); i++) {
    if (deleted[i]) continue;

    while (succ[i].size() == 1 && pred[succ[i][0]].size() == 1) {
      // Merge i with j
      auto j = succ[i][0];

      // Remove any phi and then rewrite them to be the phi_member's wrapped
      // value
      {
        auto& end_segment = basic_blocks[i].Segments().back();
        auto& start_segment = basic_blocks[j].Segments().front();
        int instr_offset = 0;
        while (start_segment.first + instr_offset <= start_segment.second &&
               OpcodeFrom(GenericInstructionReader(
                              instrs[start_segment.first + instr_offset])
                              .Opcode()) == Opcode::PHI) {
          auto reader = Type2InstructionReader(
              instrs[end_segment.second - 1 - instr_offset]);
          if (OpcodeFrom(reader.Opcode()) != Opcode::PHI_MEMBER) {
            throw std::runtime_error("Missing phi member");
          }
          auto phi_value = reader.Arg0();
          auto phi_member_value = reader.Arg1();
          value_map[phi_value] = phi_member_value;
          instr_offset++;
        }

        // Delete all phi members/branch
        end_segment = {end_segment.first,
                       end_segment.second - instr_offset - 1};

        // Delete phi
        start_segment = {start_segment.first + instr_offset,
                         start_segment.second};
      }

      if (basic_blocks[i].Segments().back().first >
          basic_blocks[i].Segments().back().second) {
        basic_blocks[i].Segments().pop_back();
      }

      auto it = basic_blocks[j].Segments().begin();
      if (it->first > it->second) {
        ++it;
      }

      // merge all segments
      basic_blocks[i].Segments().insert(basic_blocks[i].Segments().end(), it,
                                        basic_blocks[j].Segments().end());

      // delete block j
      deleted[j] = true;

      // set successors for block i to be those of block j
      succ[i] = succ[j];

      // set predecessors of successors to be i
      for (auto next : succ[i]) {
        for (auto& prev : pred[next]) {
          if (prev == j) {
            prev = i;
          }
        }
      }
    }
  }

  // rewrite all instructions to use the changed values
  for (int i = 0; i < basic_blocks.size(); i++) {
    if (deleted[i]) continue;
    for (auto [seg_start, seg_end] : basic_blocks[i].Segments()) {
      for (auto instr_idx = seg_start; instr_idx <= seg_end; instr_idx++) {
        RewriteInstr(instrs[instr_idx], value_map);
      }
    }
  }

  for (int i = 0; i < basic_blocks.size(); i++) {
    if (!deleted[i]) {
      basic_blocks[i].SetSuccessors(std::move(succ[i]));
      basic_blocks[i].SetPredecessors(std::move(pred[i]));
    }
  }
}

std::vector<BasicBlock> CFGSimplify(std::vector<uint64_t>& instrs,
                                    std::vector<BasicBlock> basic_blocks) {
  // Step 1: Remove any unreachable blocks
  auto deleted = GetDeletedBlocks(basic_blocks);

  // Step 2: Merge basic blocks
  MergeBasicBlocks(instrs, basic_blocks, deleted);

  // Step 3: Generate output
  absl::flat_hash_map<int, int> bb_idx;
  int new_idx = 0;
  for (int i = 0; i < basic_blocks.size(); i++) {
    if (deleted[i]) continue;
    bb_idx[i] = new_idx++;
  }

  std::vector<BasicBlock> output;
  for (int i = 0; i < basic_blocks.size(); i++) {
    if (deleted[i]) continue;
    RewriteBlock(basic_blocks[i], instrs, deleted, bb_idx);
    output.push_back(std::move(basic_blocks[i]));
  }

  return output;
}

}  // namespace kush::khir