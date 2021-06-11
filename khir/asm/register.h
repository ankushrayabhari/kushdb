#pragma once

#include "asmjit/x86.h"

namespace khir::khir {

class Register {
 public:
  Register(asmjit::x86::Gpq, asmjit::x86::Gpd, asmjit::x86::Gpw,
           asmjit::x86::Gpb);
  asmjit::x86::Gpq GetQ();
  asmjit::x86::Gpd GetD();
  asmjit::x86::Gpw GetW();
  asmjit::x86::Gpb GetB();

  static Register RAX;
  static Register RBX;
  static Register RCX;
  static Register RDX;
  static Register RSI;
  static Register RDI;
  static Register RSP;
  static Register RBP;
  static Register R8;
  static Register R9;
  static Register R10;
  static Register R11;
  static Register R12;
  static Register R13;
  static Register R14;
  static Register R15;

 private:
  asmjit::x86::Gpq qword_;
  asmjit::x86::Gpd dword_;
  asmjit::x86::Gpw word_;
  asmjit::x86::Gpb byte_;
};

}  // namespace khir::khir