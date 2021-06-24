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
  for (int i = 0; i < 7; i++) {
    free_floating_point_regs.insert(i);
  }

  /*
  Available for allocation:
    RBX, RCX, RDX, RSI, RDI, R8, R9, R10, R11, R12, R13, R14, R15
    XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6

  Reserved/Scratch
    RSP, RBP, RAX, XMM7
  */
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