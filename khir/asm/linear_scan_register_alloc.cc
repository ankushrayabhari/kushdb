#include "khir/asm/linear_scan_register_alloc.h"

#include <iostream>
#include <optional>
#include <vector>

#include "khir/asm/live_intervals.h"
#include "khir/asm/register.h"

namespace kush::khir {

std::vector<int> AssignRegisters(
    const std::vector<LiveInterval>& live_intervals) {
  std::vector<int> order(live_intervals.size(), -1);
  for (int i = 0; i < order.size(); i++) {
    order[i] = i;
  }

  std::sort(order.begin(), order.end(),
            [&](const int& a_idx, const int& b_idx) -> bool {
              const auto& a = live_intervals[a_idx];
              const auto& b = live_intervals[b_idx];
              if (a.Undef() && b.Undef()) {
                return false;
              }

              if (a.Undef()) {
                return false;
              }

              if (b.Undef()) {
                return true;
              }

              return a.Start() < b.Start();
            });

  std::vector<int> assignments(live_intervals.size(), -1);

  auto comp = [&](const int& a_idx, const int& b_idx) -> bool {
    const auto& a = live_intervals[a_idx];
    const auto& b = live_intervals[b_idx];
    return a.End() < b.End();
  };
  auto active_normal = std::multiset<int, decltype(comp)>(comp);
  auto active_floating_point = std::multiset<int, decltype(comp)>(comp);

  std::unordered_set<int> free_floating_point_regs;
  std::unordered_set<int> free_normal_regs;

  for (int i = 0; i < 12; i++) {
    free_normal_regs.insert(i);
  }
  for (int i = 0; i < 7; i++) {
    free_floating_point_regs.insert(i);
  }

  /*
  Available for allocation:
    RBX, RCX, RSI, RDI, R8, R9, R10, R11, R12, R13, R14, R15
    XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7

  Reserved/Scratch
    RSP, RBP, RAX, RDX, XMM0
  */
  for (int i : order) {
    assert(free_normal_regs.size() + active_normal.size() == 12);
    assert(free_floating_point_regs.size() + active_floating_point.size() == 7);

    if (live_intervals[i].Undef()) {
      break;
    }

    {  // Expire old intervals
      for (auto it = active_floating_point.begin();
           it != active_floating_point.end();) {
        auto j = *it;

        if (live_intervals[j].End() >= live_intervals[i].Start()) {
          break;
        }

        // free the register associated with this
        assert(assignments[j] >= 0);
        free_floating_point_regs.insert(assignments[j]);

        // delete live interval from active
        it = active_floating_point.erase(it);
      }

      for (auto it = active_normal.begin(); it != active_normal.end();) {
        auto j = *it;

        if (live_intervals[j].End() >= live_intervals[i].Start()) {
          break;
        }

        // free the register associated with this
        assert(assignments[j] >= 0);
        free_normal_regs.insert(assignments[j]);

        // delete live interval from active
        it = active_normal.erase(it);
      }
    }

    // free floating point registers
    if (live_intervals[i].IsFloatingPoint()) {
      if (free_floating_point_regs.empty()) {
        // Spill one of the floating point regs
        auto spill = *active_floating_point.cbegin();
        if (live_intervals[spill].End() > live_intervals[i].End()) {
          assignments[i] = assignments[spill];
          assignments[spill] = -1;
          active_floating_point.erase(spill);
          active_floating_point.insert(i);
        } else {
          assignments[i] = -1;
        }
      } else {
        auto reg = *free_floating_point_regs.begin();
        free_floating_point_regs.erase(reg);
        active_floating_point.insert(i);
        assignments[i] = reg;
      }
    } else {
      if (free_normal_regs.empty()) {
        // Spill one of the normal regs
        auto spill = *active_normal.cbegin();
        if (live_intervals[spill].End() > live_intervals[i].End()) {
          assignments[i] = assignments[spill];
          assignments[spill] = -1;
          active_normal.erase(spill);
          active_normal.insert(i);
        } else {
          assignments[i] = -1;
        }
      } else {
        auto reg = *free_normal_regs.begin();
        free_normal_regs.erase(reg);
        active_normal.insert(i);
        assignments[i] = reg;
      }
    }
  }

  /*
  for (int i = 0; i < live_intervals.size(); i++) {
    std::cerr << "%" << i << " " << assignments[i] << "\n";
  }
  */

  return assignments;
}

}  // namespace kush::khir