#pragma once

#include <vector>

#include "khir/asm/rpo_label.h"
#include "khir/program_builder.h"

namespace kush::khir {

class LiveInterval {
 public:
  LiveInterval(khir::Value v, khir::Type t);
  LiveInterval(int reg);
  void Extend(int bb, int idx);

  void ChangeToPrecolored(int reg);

  int StartBB() const;
  int EndBB() const;
  int StartIdx() const;
  int EndIdx() const;

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
  int start_bb_;
  int end_bb_;
  int start_idx_;
  int end_idx_;

  khir::Value value_;
  khir::Type type_;
  int register_;

  int spill_cost_;
};

std::vector<LiveInterval> ComputeLiveIntervals(const Function& func,
                                               const TypeManager& manager,
                                               const RPOLabelResult& rpo);

}  // namespace kush::khir