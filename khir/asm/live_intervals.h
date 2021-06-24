#pragma once

#include <vector>

#include "khir/program_builder.h"

namespace kush::khir {

class LiveInterval {
 public:
  LiveInterval(khir::Value v, khir::Type t);
  LiveInterval(int reg);
  void Extend(int bb, int idx);

  int StartBB() const;
  int EndBB() const;
  int StartIdx() const;
  int EndIdx() const;

  bool Undef() const;
  khir::Type Type() const;
  khir::Value Value() const;
  int Register() const;
  bool IsRegister() const;

 private:
  bool undef_;
  int start_bb_;
  int end_bb_;
  int start_idx_;
  int end_idx_;

  khir::Value value_;
  khir::Type type_;
  int register_;
};

std::pair<std::vector<LiveInterval>, std::vector<int>> ComputeLiveIntervals(
    const Function& func, const TypeManager& manager);

}  // namespace kush::khir