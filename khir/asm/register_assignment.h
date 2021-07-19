#pragma once

namespace kush::khir {

class RegisterAssignment {
 public:
  RegisterAssignment(int reg, bool coalesced);

  void SetRegister(int r);

  int Register() const;
  bool IsRegister() const;

 private:
  int register_;
};

}  // namespace kush::khir