#pragma once

#include <vector>

#include "khir/program.h"

namespace kush::khir {

class LiveInterval {
 public:
  LiveInterval(khir::Value v, khir::Type t);
  LiveInterval(int reg);
  void Extend(int x);

  void ChangeToPrecolored(int reg);

  int Start() const;
  int End() const;

  bool Undef() const;
  khir::Type Type() const;
  khir::Value Value() const;

  int PrecoloredRegister() const;
  bool IsPrecolored() const;

  int SpillCost() const;
  void UpdateSpillCostWithUse(int loop_depth);

  bool operator==(const LiveInterval& rhs);

 private:
  bool undef_;
  int start_;
  int end_;

  khir::Value value_;
  khir::Type type_;
  int register_;

  int spill_cost_;
};

std::vector<LiveInterval> ComputeLiveIntervals(
    const Function& func, const std::vector<bool>& materialize_gep,
    const TypeManager& manager);

}  // namespace kush::khir