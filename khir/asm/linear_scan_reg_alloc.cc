#include "khir/asm/linear_scan_reg_alloc.h"

#include <iostream>
#include <unordered_set>
#include <vector>

#include "khir/asm/dfs_label.h"
#include "khir/asm/live_intervals.h"
#include "khir/asm/register_assignment.h"
#include "khir/instruction.h"
#include "khir/opcode.h"

namespace kush::khir {

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
    const Function& func, const TypeManager& manager) {
  auto instrs = func.Instrs();
  auto live_intervals = ComputeLiveIntervals(func, manager);

  /*
  for (auto& x : live_intervals) {
    if (x.IsPrecolored()) {
      std::cerr << "Precolored: " << x.PrecoloredRegister() << std::endl;
    } else {
      std::cerr << "Value: " << x.Value().GetIdx() << std::endl;
    }
    std::cerr << ' ' << x.Start() << ' ' << x.End() << std::endl;
  }
  */

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

  for (int a = 0; a < live_intervals.size(); a++) {
    assert(Union(active_normal, free_normal_regs, assignments).size() == 13);
    assert(Union(active_floating_point, free_floating_point_regs, assignments)
               .size() == 7);

    LiveInterval& i = live_intervals[a];
    assert(!i.Undef());

    // Expire old intervals
    ExpireOldIntervals(i, assignments, free_normal_regs, active_normal);
    ExpireOldIntervals(i, assignments, free_floating_point_regs,
                       active_floating_point);

    if (i.IsPrecolored()) {
      if (i.PrecoloredRegister() >= 50) {
        AddPrecoloredInterval(i, assignments, free_floating_point_regs,
                              active_floating_point);
      } else {
        AddPrecoloredInterval(i, assignments, free_normal_regs, active_normal);
      }
    } else {
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
        case Opcode::I64_CMP_GE: {
          if (i.Start() + 1 == i.End() &&
              OpcodeFrom(
                  GenericInstructionReader(instrs[i.Value().GetIdx() + 1])
                      .Opcode()) == Opcode::CONDBR) {
            assignments[i_instr].SetRegister(100);
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
              OpcodeFrom(
                  GenericInstructionReader(instrs[i.Value().GetIdx() + 1])
                      .Opcode()) == Opcode::CONDBR) {
            assignments[i_instr].SetRegister(101);
          }
          break;
        }

        case Opcode::I8_STORE:
        case Opcode::I16_STORE:
        case Opcode::I32_STORE:
        case Opcode::I64_STORE:
        case Opcode::PTR_STORE: {
          AddPrecoloredInterval(i, assignments, free_normal_regs,
                                active_normal);
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
  }

  /*
  for (int i = 0; i < assignments.size(); i++) {
    std::cerr << "Assign: " << i << ' ' << assignments[i].Register()
              << std::endl;
  }
  */

  return assignments;
}

}  // namespace kush::khir