#pragma once

#include "asmjit/x86.h"

namespace kush::khir {

class GPRegister {
 public:
  GPRegister(asmjit::x86::Gpq, asmjit::x86::Gpd, asmjit::x86::Gpw,
             asmjit::x86::GpbLo);
  asmjit::x86::Gpq GetQ() const;
  asmjit::x86::Gpd GetD() const;
  asmjit::x86::Gpw GetW() const;
  asmjit::x86::GpbLo GetB() const;

  static GPRegister RAX;
  static GPRegister RBX;
  static GPRegister RCX;
  static GPRegister RDX;
  static GPRegister RSI;
  static GPRegister RDI;
  static GPRegister RSP;
  static GPRegister RBP;
  static GPRegister R8;
  static GPRegister R9;
  static GPRegister R10;
  static GPRegister R11;
  static GPRegister R12;
  static GPRegister R13;
  static GPRegister R14;
  static GPRegister R15;

 private:
  asmjit::x86::Gpq qword_;
  asmjit::x86::Gpd dword_;
  asmjit::x86::Gpw word_;
  asmjit::x86::GpbLo byte_;
};

}  // namespace kush::khir