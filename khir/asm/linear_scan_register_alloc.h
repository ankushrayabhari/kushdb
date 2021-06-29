#pragma once

#include <optional>
#include <vector>

#include "khir/asm/live_intervals.h"
#include "khir/asm/register.h"

namespace kush::khir {

class RegisterAssignment {
 public:
  RegisterAssignment(int reg, bool coalesced);

  void SetRegister(int r);
  void SetCoalseced(bool c);

  int Register() const;
  bool IsCoalesced() const;
  bool IsSpilled() const;

 private:
  int register_;
  bool coalesced_;
};

std::vector<RegisterAssignment> AssignRegisters(
    std::vector<LiveInterval>& live_intervals,
    const std::vector<uint64_t>& instrs, const TypeManager& manager);

}  // namespace kush::khir