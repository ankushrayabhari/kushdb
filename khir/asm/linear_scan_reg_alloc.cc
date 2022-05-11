#include "khir/asm/linear_scan_reg_alloc.h"

#include <iostream>
#include <unordered_set>
#include <vector>

#include "khir/asm/dfs_label.h"
#include "khir/asm/live_intervals.h"
#include "khir/asm/register.h"
#include "khir/asm/register_assignment.h"
#include "khir/instruction.h"
#include "khir/opcode.h"

namespace kush::khir {

bool IsVector(const TypeManager& manager, khir::Type t) {
  return manager.IsF64Type(t) || manager.IsI32Vec8Type(t);
}

template <typename ActiveSet>
std::unordered_set<int> Union(ActiveSet& active, std::unordered_set<int>& free,
                              std::vector<RegisterAssignment>& assignments) {
  std::unordered_set<int> res = free;
  for (const auto& x : active) {
    if (x.IsPrecolored()) {
      res.insert(x.PrecoloredRegister());
    } else {
      assert(assignments[x.Value().GetIdx()].IsRegister());
      res.insert(assignments[x.Value().GetIdx()].Register());
    }
  }
  return res;
}

template <typename ActiveSet>
int SpillMinCost(ActiveSet& active, std::unordered_set<int>& free,
                 std::vector<RegisterAssignment>& assignments) {
  if (active.empty()) {
    throw std::runtime_error("Nothing to spill.");
  }

  bool undef = true;
  auto min_spill_cost = INT32_MAX;
  auto min_spill_it = active.end();
  for (auto it = active.begin(); it != active.end(); it++) {
    if (it->IsPrecolored()) {
      continue;
    }

    auto spill_cost = it->SpillCost();
    if (undef) {
      undef = false;
      min_spill_cost = spill_cost;
      min_spill_it = it;
    } else if (spill_cost < min_spill_cost) {
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

      assignments[j_idx].Spill();
      active.erase(it);
      active.insert(to_add);
      return;
    }

    throw std::runtime_error("Register should have been in free: " +
                             std::to_string(reg));
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
    if (j.End() >= current.Start()) {
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

std::vector<RegisterAssignment> LinearScanRegisterAlloc(
    const Function& func, const std::vector<bool>& materialize_gep,
    const TypeManager& manager) {
  auto instrs = func.Instrs();
  auto live_intervals = ComputeLiveIntervals(func, materialize_gep, manager);

  // Handle intervals by increasing start point order
  std::sort(live_intervals.begin(), live_intervals.end(),
            [](const LiveInterval& a, const LiveInterval& b) -> bool {
              return a.Start() < b.Start();
            });

  std::vector<RegisterAssignment> assignments(instrs.size());

  // Keep track of intervals by increasing end point
  auto comp = [&](const LiveInterval& a, const LiveInterval& b) -> bool {
    return a.End() < b.End();
  };
  auto active_normal = std::multiset<LiveInterval, decltype(comp)>(comp);
  auto active_vector = std::multiset<LiveInterval, decltype(comp)>(comp);

  /*
  Available for allocation:
    RBX, RCX, RSI, RDI, R8, R9, R10, R11, R12, R13, R14, R15
    XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7, XMM8, XMM9, XMM10, XMM11,
    XMM12, XMM13, XMM14

  Reserved/Scratch
    RSP, RBP, RAX, xmm15
  */

  std::unordered_set<int> free_vector;
  free_vector.insert(VRegister::M0.Id());
  free_vector.insert(VRegister::M1.Id());
  free_vector.insert(VRegister::M2.Id());
  free_vector.insert(VRegister::M3.Id());
  free_vector.insert(VRegister::M4.Id());
  free_vector.insert(VRegister::M5.Id());
  free_vector.insert(VRegister::M6.Id());
  free_vector.insert(VRegister::M7.Id());
  free_vector.insert(VRegister::M8.Id());
  free_vector.insert(VRegister::M9.Id());
  free_vector.insert(VRegister::M10.Id());
  free_vector.insert(VRegister::M11.Id());
  free_vector.insert(VRegister::M12.Id());
  free_vector.insert(VRegister::M13.Id());
  free_vector.insert(VRegister::M14.Id());

  std::unordered_set<int> free_normal;
  free_normal.insert(GPRegister::RBX.Id());
  free_normal.insert(GPRegister::RCX.Id());
  free_normal.insert(GPRegister::RDX.Id());
  free_normal.insert(GPRegister::RSI.Id());
  free_normal.insert(GPRegister::RDI.Id());
  free_normal.insert(GPRegister::R8.Id());
  free_normal.insert(GPRegister::R9.Id());
  free_normal.insert(GPRegister::R10.Id());
  free_normal.insert(GPRegister::R11.Id());
  free_normal.insert(GPRegister::R12.Id());
  free_normal.insert(GPRegister::R13.Id());
  free_normal.insert(GPRegister::R14.Id());
  free_normal.insert(GPRegister::R15.Id());

  const int TOTAL_GP_FREE = free_normal.size();
  const int TOTAL_V_FREE = free_vector.size();

  for (int a = 0; a < live_intervals.size(); a++) {
    assert(Union(active_normal, free_normal, assignments).size() ==
           TOTAL_GP_FREE);
    assert(Union(active_vector, free_vector, assignments).size() ==
           TOTAL_V_FREE);

    LiveInterval& i = live_intervals[a];
    assert(!i.Undef());

    // Expire old intervals
    ExpireOldIntervals(i, assignments, free_normal, active_normal);
    ExpireOldIntervals(i, assignments, free_vector, active_vector);

    if (i.IsPrecolored()) {
      if (VRegister::IsVRegister(i.PrecoloredRegister())) {
        AddPrecoloredInterval(i, assignments, free_vector, active_vector);
      } else if (GPRegister::IsGPRegister(i.PrecoloredRegister())) {
        AddPrecoloredInterval(i, assignments, free_normal, active_normal);
      } else {
        throw std::runtime_error("Invalid precolored register");
      }
      continue;
    }

    auto i_instr = i.Value().GetIdx();
    auto i_opcode =
        OpcodeFrom(GenericInstructionReader(instrs[i_instr]).Opcode());

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
      case Opcode::I64_CMP_GE:
      case Opcode::I32_CMP_EQ_ANY_CONST_VEC4:
      case Opcode::I32_CMP_EQ_ANY_CONST_VEC8: {
        if (i.Start() + 1 == i.End() &&
            OpcodeFrom(GenericInstructionReader(instrs[i.Value().GetIdx() + 1])
                           .Opcode()) == Opcode::CONDBR) {
          assignments[i_instr].SetRegister(FRegister::IFlag.Id());
        }
        break;
      }

      case Opcode::F64_CMP_EQ:
      case Opcode::F64_CMP_NE:
      case Opcode::F64_CMP_LT:
      case Opcode::F64_CMP_LE:
      case Opcode::F64_CMP_GT:
      case Opcode::F64_CMP_GE: {
        if (i.Start() + 1 == i.End() &&
            OpcodeFrom(GenericInstructionReader(instrs[i.Value().GetIdx() + 1])
                           .Opcode()) == Opcode::CONDBR) {
          assignments[i_instr].SetRegister(FRegister::FFlag.Id());
        }
        break;
      }

      case Opcode::I8_STORE:
      case Opcode::I16_STORE:
      case Opcode::I32_STORE:
      case Opcode::I64_STORE:
      case Opcode::PTR_STORE: {
        AddPrecoloredInterval(i, assignments, free_normal, active_normal);
        assert(i.IsPrecolored());
        assignments[i_instr].SetRegister(i.PrecoloredRegister());
        break;
      }

      default: {
        if (IsVector(manager, i.Type())) {
          SpillAtInterval(i, assignments, free_vector, active_vector);
        } else {
          SpillAtInterval(i, assignments, free_normal, active_normal);
        }
        break;
      }
    }
  }

  return assignments;
}

}  // namespace kush::khir