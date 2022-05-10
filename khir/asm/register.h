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

class VRegister {
 public:
  VRegister(asmjit::x86::Ymm, asmjit::x86::Xmm);
  asmjit::x86::Xmm GetX() const;
  asmjit::x86::Ymm GetY() const;

  static VRegister M0;
  static VRegister M1;
  static VRegister M2;
  static VRegister M3;
  static VRegister M4;
  static VRegister M5;
  static VRegister M6;
  static VRegister M7;
  static VRegister M8;
  static VRegister M9;
  static VRegister M10;
  static VRegister M11;
  static VRegister M12;
  static VRegister M13;
  static VRegister M14;
  static VRegister M15;

 private:
  asmjit::x86::Ymm ymm_;
  asmjit::x86::Xmm xmm_;
};

}  // namespace kush::khir