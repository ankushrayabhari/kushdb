#include "khir/asm/linear_scan_register_alloc.h"

#include <iostream>
#include <optional>
#include <vector>

#include "khir/asm/live_intervals.h"
#include "khir/asm/register.h"
#include "khir/instruction.h"
#include "khir/opcode.h"

namespace kush::khir {

std::vector<int> AssignRegisters(std::vector<LiveInterval>& live_intervals,
                                 const std::vector<uint64_t>& instrs,
                                 const TypeManager& manager) {
  // Sort by increasing start point order
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

  std::vector<int> assignments(instrs.size(), -1);

  // Sort by increasing end point
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
  std::unordered_set<int> free_normal_regs;

  for (int i = 0; i < 13; i++) {
    free_normal_regs.insert(i);
  }
  for (int i = 50; i < 50 + 7; i++) {
    free_floating_point_regs.insert(i);
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
      active_normal.insert(interval);
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

  for (const auto& i : live_intervals) {
    assert(!i.Undef());
    assert(free_normal_regs.size() + active_normal.size() == 13);
    assert(free_floating_point_regs.size() + active_floating_point.size() == 7);

    {  // Expire old intervals

      // Normal Regs
      for (auto it = active_normal.begin(); it != active_normal.end();) {
        const auto& j = *it;

        // Stop once the endpoint[j] >= startpoint[i]
        if (j.EndBB() > i.StartBB()) {
          break;
        }

        if (j.EndBB() == i.StartBB() && j.EndIdx() >= i.StartIdx()) {
          break;
        }

        // Free the register associated with this interval
        int reg =
            j.IsRegister() ? j.Register() : assignments[j.Value().GetIdx()];
        assert(reg >= 0);
        free_normal_regs.insert(reg);

        // Delete live interval from active
        it = active_normal.erase(it);
      }

      // FP Regs
      for (auto it = active_floating_point.begin();
           it != active_floating_point.end();) {
        const auto& j = *it;

        // Stop once the endpoint[j] >= startpoint[i]
        if (j.EndBB() > i.StartBB()) {
          break;
        }

        if (j.EndBB() == i.StartBB() && j.EndIdx() >= i.StartIdx()) {
          break;
        }

        // Free the register associated with this interval
        int reg =
            j.IsRegister() ? j.Register() : assignments[j.Value().GetIdx()];
        assert(reg >= 0);
        free_floating_point_regs.insert(reg);

        // delete live interval from active
        it = active_floating_point.erase(it);
      }
    }

    // Can't be a precolored interval. Needs to be virtual.
    assert(!i.IsRegister());
    auto i_instr = i.Value().GetIdx();

    // FLAG register into branch
    if (manager.IsI1Type(i.Type())) {
      if (i.StartBB() == i.EndBB() && i.StartIdx() + 1 == i.EndIdx() &&
          OpcodeFrom(GenericInstructionReader(instrs[i.EndIdx()]).Opcode()) ==
              Opcode::CONDBR) {
        assignments[i_instr] = 100;
        continue;
      }
    }

    if (manager.IsF64Type(i.Type())) {
      if (free_floating_point_regs.empty()) {
        // Spill one of the non-fixed floating point regs
        auto spill = active_floating_point.begin();
        while (spill != active_floating_point.end()) {
          if (spill->IsRegister()) {
            spill++;
            continue;
          }

          break;
        }

        if (spill == active_floating_point.end()) {
          // Forced to spill the current.
          assignments[i_instr] = -1;
          continue;
        }
        assert(!spill->IsRegister());
        auto spill_instr = spill->Value().GetIdx();

        // Heuristic.
        if (spill->EndBB() > i.EndBB() ||
            (spill->EndBB() == i.EndBB() && spill->EndIdx() > i.EndIdx())) {
          assignments[i_instr] = assignments[spill_instr];
          assignments[spill_instr] = -1;
          active_floating_point.erase(spill);
          active_floating_point.insert(i);
        } else {
          assignments[i_instr] = -1;
        }

        continue;
      }

      // Free register available
      int reg = *free_floating_point_regs.begin();
      free_floating_point_regs.erase(reg);
      active_floating_point.insert(i);
      assignments[i_instr] = reg;
      continue;
    }

    // Normal Register
    if (free_normal_regs.empty()) {
      // Spill one of the non-fixed normal regs
      auto spill = active_normal.begin();
      while (spill != active_normal.end()) {
        if (spill->IsRegister()) {
          spill++;
          continue;
        }

        break;
      }

      if (spill == active_normal.end()) {
        // Forced to spill the current.
        assignments[i_instr] = -1;
        continue;
      }
      assert(!spill->IsRegister());
      auto spill_instr = spill->Value().GetIdx();

      // Heuristic.
      if (spill->EndBB() > i.EndBB() ||
          (spill->EndBB() == i.EndBB() && spill->EndIdx() > i.EndIdx())) {
        assignments[i_instr] = assignments[spill_instr];
        assignments[spill_instr] = -1;
        active_normal.erase(spill);
        active_normal.insert(i);
      } else {
        assignments[i_instr] = -1;
      }
      continue;
    }

    // Free register available
    assert(!free_normal_regs.empty());
    int reg = *free_normal_regs.begin();
    free_normal_regs.erase(reg);
    active_normal.insert(i);
    assignments[i_instr] = reg;
    continue;
  }

  for (int i = 0; i < instrs.size(); i++) {
    std::cerr << "%" << i << " " << assignments[i] << "\n";
  }

  return assignments;
}

}  // namespace kush::khir