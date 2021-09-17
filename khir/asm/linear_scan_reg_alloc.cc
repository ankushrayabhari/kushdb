#include "khir/asm/linear_scan_reg_alloc.h"

#include <iostream>
#include <unordered_set>
#include <vector>

#include "khir/asm/bb_label.h"
#include "khir/asm/live_intervals.h"
#include "khir/asm/register_assignment.h"
#include "khir/instruction.h"
#include "khir/opcode.h"

namespace kush::khir {

template <typename ActiveSet>
int SpillMinCost(ActiveSet& active, std::unordered_set<int>& free,
                 std::vector<RegisterAssignment>& assignments) {
  if (active.empty()) {
    throw std::runtime_error("Nothing to spill.");
  }

  auto min_spill_cost = INT32_MAX;
  auto min_spill_it = active.end();
  for (auto it = active.begin(); it != active.end(); it++) {
    if (it->IsPrecolored()) {
      continue;
    }

    auto spill_cost = it->SpillCost();
    if (spill_cost < min_spill_cost) {
      min_spill_cost = spill_cost;
      min_spill_it = it;
    }
  }

  if (min_spill_it == active.end()) {
    throw std::runtime_error("No spillable register exists");
  }

  auto reg = assignments[min_spill_it->Value().GetIdx()].Register();
  assignments[min_spill_it->Value().GetIdx()].Spill();
  free.insert(reg);
  active.erase(min_spill_it);
  return reg;
}

template <typename ActiveSet>
int MaybeSpillMinCost(ActiveSet& active, std::unordered_set<int>& free,
                      std::vector<RegisterAssignment>& assignments,
                      const LiveInterval& maybe) {
  if (active.empty()) {
    throw std::runtime_error("Nothing to spill.");
  }

  auto min_spill_cost = maybe.SpillCost();
  auto min_spill_it = active.end();
  for (auto it = active.begin(); it != active.end(); it++) {
    if (it->IsPrecolored()) {
      continue;
    }

    auto spill_cost = it->SpillCost();
    if (spill_cost < min_spill_cost) {
      min_spill_cost = spill_cost;
      min_spill_it = it;
    }
  }

  if (min_spill_it == active.end()) {
    // nothing has lower spill cost
    return -1;
  }

  auto reg = assignments[min_spill_it->Value().GetIdx()].Register();
  assignments[min_spill_it->Value().GetIdx()].Spill();
  free.insert(reg);
  active.erase(min_spill_it);
  return reg;
}

template <typename ActiveSet>
void AddPrecoloredInterval(LiveInterval& to_add,
                           std::vector<RegisterAssignment>& assignments,
                           std::unordered_set<int>& free, ActiveSet& active) {
  // Constraint on the register
  if (to_add.IsPrecolored()) {
    auto reg = to_add.PrecoloredRegister();
    if (free.find(reg) != free.end()) {
      free.erase(reg);
      active.insert(to_add);
      return;
    }

    // Find the interval in active that is assigned to reg
    for (auto it = active.begin(); it != active.end(); it++) {
      const auto& j = *it;
      int j_reg = j.IsPrecolored() ? j.PrecoloredRegister()
                                   : assignments[j.Value().GetIdx()].Register();
      if (reg != j_reg) {
        continue;
      }
      if (j.IsPrecolored()) {
        throw std::runtime_error("Two precolored intervals conflicting.");
      }

      auto j_idx = j.Value().GetIdx();

      // Find the min cost register in active
      auto free_reg = SpillMinCost(active, free, assignments);
      if (free_reg == j_reg) {
        // we spilled j
        assert(!assignments[j_idx].IsRegister());
        active.insert(to_add);
        free.erase(reg);
      } else {
        // we spilled something else
        // move j to that register
        assignments[j_idx].SetRegister(free_reg);
        free.erase(free_reg);

        // now that j's old register is free, we can add to_add
        active.insert(to_add);
      }
      return;
    }

    throw std::runtime_error("Register should have been in free.");
  }

  // No constraint on the register, pick any register
  if (!free.empty()) {
    // Free register available
    int reg = *free.begin();

    to_add.ChangeToPrecolored(reg);
    active.insert(to_add);
    free.erase(reg);
    return;
  }

  // pick the min cost one to spill and spill that
  // let to_add be precolored with that free_reg
  auto free_reg = SpillMinCost(active, free, assignments);
  to_add.ChangeToPrecolored(free_reg);
  active.insert(to_add);
  free.erase(free_reg);
}

template <typename ActiveSet>
void ExpireOldIntervals(LiveInterval& current,
                        std::vector<RegisterAssignment>& assignments,
                        std::unordered_set<int>& free, ActiveSet& active) {
  // Normal Regs
  for (auto it = active.begin(); it != active.end();) {
    const auto& j = *it;

    // Stop once the endpoint[j] >= startpoint[i]
    if (j.EndBB() > current.StartBB()) {
      break;
    }

    if (j.EndBB() == current.StartBB() && j.EndIdx() >= current.StartIdx()) {
      break;
    }

    // Free the register associated with this interval
    if (j.IsPrecolored()) {
      free.insert(j.PrecoloredRegister());
    } else {
      assert(assignments[j.Value().GetIdx()].IsRegister());
      free.insert(assignments[j.Value().GetIdx()].Register());
    }

    // Delete live interval from active
    it = active.erase(it);
  }
}

template <typename ActiveSet>
void SpillAtInterval(LiveInterval& curr,
                     std::vector<RegisterAssignment>& assignments,
                     std::unordered_set<int>& free, ActiveSet& active) {
  auto curr_idx = curr.Value().GetIdx();

  if (!free.empty()) {
    // Free register available
    int reg = *free.begin();
    free.erase(reg);
    active.insert(curr);
    assignments[curr_idx].SetRegister(reg);
    return;
  }

  // spill live interval with min cost only if smaller than the live interval
  auto free_reg = MaybeSpillMinCost(active, free, assignments, curr);
  if (free_reg == -1) {
    // spill current
    assignments[curr_idx].Spill();
  } else {
    assignments[curr_idx].SetRegister(free_reg);
    free.erase(free_reg);
    active.insert(curr);
  }
}

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

std::vector<RegisterAssignment> LinearScanRegisterAlloc(
    const Function& func, const TypeManager& manager,
    const BBLabelResult& rpo) {
  auto instrs = func.Instructions();
  auto live_intervals = ComputeLiveIntervals(func, manager, rpo);

  // Handle intervals by increasing start point order
  std::sort(live_intervals.begin(), live_intervals.end(),
            [](const LiveInterval& a, const LiveInterval& b) -> bool {
              if (a.StartBB() < b.StartBB()) {
                return true;
              }

              if (a.StartBB() == b.StartBB()) {
                return a.StartIdx() < b.StartIdx();
              }

              return false;
            });

  std::vector<RegisterAssignment> assignments(instrs.size());

  // Keep track of intervals by increasing end point
  auto comp = [&](const LiveInterval& a, const LiveInterval& b) -> bool {
    if (a.EndBB() < b.EndBB()) {
      return true;
    }

    if (a.EndBB() == b.EndBB()) {
      return a.EndIdx() < b.EndIdx();
    }

    return false;
  };
  auto active_normal = std::multiset<LiveInterval, decltype(comp)>(comp);
  auto active_floating_point =
      std::multiset<LiveInterval, decltype(comp)>(comp);

  std::unordered_set<int> free_floating_point_regs;
  for (int i = 50; i < 50 + 7; i++) {
    free_floating_point_regs.insert(i);
  }

  std::unordered_set<int> free_normal_regs;
  for (int i = 0; i < 13; i++) {
    free_normal_regs.insert(i);
  }

  // Create a precolored interval for each function argument
  std::vector<int> normal_arg_reg{4, 3, 2, 1, 5, 6};
  std::vector<int> fp_arg_reg{50, 51, 52, 53, 54, 55, 56};
  for (int i = 0, normal_arg_ctr = 0, fp_arg_ctr = 0; i < instrs.size(); i++) {
    auto opcode = OpcodeFrom(GenericInstructionReader(instrs[i]).Opcode());
    if (opcode != Opcode::FUNC_ARG) {
      break;
    }

    Type3InstructionReader reader(instrs[i]);
    auto type = static_cast<Type>(reader.TypeID());
    if (manager.IsF64Type(type)) {
      auto reg = fp_arg_reg[fp_arg_ctr++];
      LiveInterval interval(reg);
      interval.Extend(0, 0);
      interval.Extend(0, i);
      active_floating_point.insert(interval);
      free_floating_point_regs.erase(reg);
    } else {
      auto reg = normal_arg_reg[normal_arg_ctr++];
      LiveInterval interval(reg);
      interval.Extend(0, 0);
      interval.Extend(0, i);
      active_normal.insert(interval);
      free_normal_regs.erase(reg);
    }
  }

  int normal_arg_ctr = 0;
  int fp_arg_ctr = 0;

  for (int a = 0; a < live_intervals.size(); a++) {
    LiveInterval& i = live_intervals[a];
    assert(!i.Undef());
    assert(free_normal_regs.size() + active_normal.size() == 13);
    assert(free_floating_point_regs.size() + active_floating_point.size() == 7);

    // Can't be a precolored interval.
    assert(!i.IsPrecolored());
    auto i_instr = i.Value().GetIdx();
    auto i_opcode =
        OpcodeFrom(GenericInstructionReader(instrs[i_instr]).Opcode());

    // Expire old intervals
    ExpireOldIntervals(i, assignments, free_normal_regs, active_normal);
    ExpireOldIntervals(i, assignments, free_floating_point_regs,
                       active_floating_point);

    // Handle CALL_ARG as a precolored reg
    if (i_opcode == Opcode::CALL_ARG) {
      if (manager.IsF64Type(i.Type())) {
        // Passing argument by stack so just move on.
        if (fp_arg_ctr >= fp_arg_reg.size()) {
          fp_arg_ctr++;
        } else {
          int reg = fp_arg_reg[fp_arg_ctr++];
          assignments[i_instr].SetRegister(reg);
          i.ChangeToPrecolored(reg);
          AddPrecoloredInterval(i, assignments, free_floating_point_regs,
                                active_floating_point);
        }
      } else {
        // Passing argument by stack so just move on.
        if (normal_arg_ctr >= normal_arg_reg.size()) {
          normal_arg_ctr++;
        } else {
          int reg = normal_arg_reg[normal_arg_ctr++];
          assignments[i_instr].SetRegister(reg);
          i.ChangeToPrecolored(reg);
          AddPrecoloredInterval(i, assignments, free_normal_regs,
                                active_normal);
        }
      }

      auto next_opcode = OpcodeFrom(
          GenericInstructionReader(instrs[i.StartIdx() + 1]).Opcode());
      if (next_opcode == Opcode::CALL || next_opcode == Opcode::CALL_INDIRECT) {
        // add in live intervals for all remaining arg regs since they are
        // clobbered
        while (normal_arg_ctr < normal_arg_reg.size()) {
          int reg = normal_arg_reg[normal_arg_ctr++];
          LiveInterval interval(reg);
          interval.Extend(i.StartBB(), i.StartIdx());
          AddPrecoloredInterval(interval, assignments, free_normal_regs,
                                active_normal);
        }

        while (fp_arg_ctr < fp_arg_reg.size()) {
          int reg = fp_arg_reg[fp_arg_ctr++];
          LiveInterval interval(reg);
          interval.Extend(i.StartBB(), i.StartIdx());
          AddPrecoloredInterval(interval, assignments, free_floating_point_regs,
                                active_floating_point);
        }

        // clobber remaining caller saved registers
        // FP registers have already been clobbered.
        const std::vector<int> caller_saved_normal_registers = {7, 8};
        for (int reg : caller_saved_normal_registers) {
          LiveInterval interval(reg);
          interval.Extend(i.StartBB(), i.StartIdx());
          AddPrecoloredInterval(interval, assignments, free_normal_regs,
                                active_normal);
        }

        // reset arg counters
        normal_arg_ctr = 0;
        fp_arg_ctr = 0;
      }
      continue;
    }

    switch (i_opcode) {
      case Opcode::I1_CMP_EQ:
      case Opcode::I1_CMP_NE:
      case Opcode::I8_CMP_EQ:
      case Opcode::I8_CMP_NE:
      case Opcode::I8_CMP_LT:
      case Opcode::I8_CMP_LE:
      case Opcode::I8_CMP_GT:
      case Opcode::I8_CMP_GE:
      case Opcode::I16_CMP_EQ:
      case Opcode::I16_CMP_NE:
      case Opcode::I16_CMP_LT:
      case Opcode::I16_CMP_LE:
      case Opcode::I16_CMP_GT:
      case Opcode::I16_CMP_GE:
      case Opcode::I32_CMP_EQ:
      case Opcode::I32_CMP_NE:
      case Opcode::I32_CMP_LT:
      case Opcode::I32_CMP_LE:
      case Opcode::I32_CMP_GT:
      case Opcode::I32_CMP_GE:
      case Opcode::I64_CMP_EQ:
      case Opcode::I64_CMP_NE:
      case Opcode::I64_CMP_LT:
      case Opcode::I64_CMP_LE:
      case Opcode::I64_CMP_GT:
      case Opcode::I64_CMP_GE: {
        if (i.StartBB() == i.EndBB() && i.StartIdx() + 1 == i.EndIdx() &&
            OpcodeFrom(GenericInstructionReader(instrs[i.EndIdx()]).Opcode()) ==
                Opcode::CONDBR) {
          assignments[i_instr].SetRegister(100);
          continue;
        }
        break;
      }

      case Opcode::F64_CMP_EQ:
      case Opcode::F64_CMP_NE:
      case Opcode::F64_CMP_LT:
      case Opcode::F64_CMP_LE:
      case Opcode::F64_CMP_GT:
      case Opcode::F64_CMP_GE: {
        if (i.StartBB() == i.EndBB() && i.StartIdx() + 1 == i.EndIdx() &&
            OpcodeFrom(GenericInstructionReader(instrs[i.EndIdx()]).Opcode()) ==
                Opcode::CONDBR) {
          assignments[i_instr].SetRegister(101);
          continue;
        }
        break;
      }

      case Opcode::I8_STORE:
      case Opcode::I16_STORE:
      case Opcode::I32_STORE:
      case Opcode::I64_STORE:
      case Opcode::PTR_STORE: {
        AddPrecoloredInterval(i, assignments, free_normal_regs, active_normal);
        assert(i.IsPrecolored());
        assignments[i_instr].SetRegister(i.PrecoloredRegister());
        break;
      }

      default: {
        if (manager.IsF64Type(i.Type())) {
          SpillAtInterval(i, assignments, free_floating_point_regs,
                          active_floating_point);
        } else {
          SpillAtInterval(i, assignments, free_normal_regs, active_normal);
        }
        break;
      }
    }
  }

  return assignments;
}

}  // namespace kush::khir