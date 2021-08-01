#pragma once

namespace kush::khir {

class RegisterAssignment {
 public:
  RegisterAssignment();
  RegisterAssignment(int reg);

  void Spill();
  void SetRegister(int r);

  int Register() const;
  bool IsRegister() const;

 private:
  int register_;
};

}  // namespace kush::khir