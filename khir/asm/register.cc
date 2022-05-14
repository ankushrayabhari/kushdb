#include "khir/asm/register.h"

#include "asmjit/x86.h"

namespace kush::khir {

using namespace asmjit;

GPRegister::GPRegister(asmjit::x86::Gpq qword, asmjit::x86::Gpd dword,
                       asmjit::x86::Gpw word, asmjit::x86::GpbLo byte, int id)
    : qword_(qword), dword_(dword), word_(word), byte_(byte), id_(id) {}

asmjit::x86::Gpq GPRegister::GetQ() const { return qword_; }

asmjit::x86::Gpd GPRegister::GetD() const { return dword_; }

asmjit::x86::Gpw GPRegister::GetW() const { return word_; }

asmjit::x86::GpbLo GPRegister::GetB() const { return byte_; }

int GPRegister::Id() const { return id_; }

GPRegister GPRegister::FromId(int id) {
  switch (id) {
    case 0:
      return GPRegister::RAX;
    case 1:
      return GPRegister::RBX;
    case 2:
      return GPRegister::RCX;
    case 3:
      return GPRegister::RDX;
    case 4:
      return GPRegister::RSI;
    case 5:
      return GPRegister::RDI;
    case 6:
      return GPRegister::RSP;
    case 7:
      return GPRegister::RBP;
    case 8:
      return GPRegister::R8;
    case 9:
      return GPRegister::R9;
    case 10:
      return GPRegister::R10;
    case 11:
      return GPRegister::R11;
    case 12:
      return GPRegister::R12;
    case 13:
      return GPRegister::R13;
    case 14:
      return GPRegister::R14;
    case 15:
      return GPRegister::R15;
    default:
      throw std::runtime_error("Invalid GPRegister ID.");
  }
}

bool GPRegister::IsGPRegister(int id) { return id >= 0 && id <= 15; }

GPRegister GPRegister::RAX(x86::rax, x86::eax, x86::ax, x86::al, 0);
GPRegister GPRegister::RBX(x86::rbx, x86::ebx, x86::bx, x86::bl, 1);
GPRegister GPRegister::RCX(x86::rcx, x86::ecx, x86::cx, x86::cl, 2);
GPRegister GPRegister::RDX(x86::rdx, x86::edx, x86::dx, x86::dl, 3);
GPRegister GPRegister::RSI(x86::rsi, x86::esi, x86::si, x86::sil, 4);
GPRegister GPRegister::RDI(x86::rdi, x86::edi, x86::di, x86::dil, 5);
GPRegister GPRegister::RSP(x86::rsp, x86::esp, x86::sp, x86::spl, 6);
GPRegister GPRegister::RBP(x86::rbp, x86::ebp, x86::bp, x86::bpl, 7);
GPRegister GPRegister::R8(x86::r8, x86::r8d, x86::r8w, x86::r8b, 8);
GPRegister GPRegister::R9(x86::r9, x86::r9d, x86::r9w, x86::r9b, 9);
GPRegister GPRegister::R10(x86::r10, x86::r10d, x86::r10w, x86::r10b, 10);
GPRegister GPRegister::R11(x86::r11, x86::r11d, x86::r11w, x86::r11b, 11);
GPRegister GPRegister::R12(x86::r12, x86::r12d, x86::r12w, x86::r12b, 12);
GPRegister GPRegister::R13(x86::r13, x86::r13d, x86::r13w, x86::r13b, 13);
GPRegister GPRegister::R14(x86::r14, x86::r14d, x86::r14w, x86::r14b, 14);
GPRegister GPRegister::R15(x86::r15, x86::r15d, x86::r15w, x86::r15b, 15);

VRegister::VRegister(asmjit::x86::Ymm ymm, asmjit::x86::Xmm xmm, int id)
    : ymm_(ymm), xmm_(xmm), id_(id) {}

asmjit::x86::Xmm VRegister::GetX() const { return xmm_; }

asmjit::x86::Ymm VRegister::GetY() const { return ymm_; }

VRegister VRegister::FromId(int id) {
  switch (id) {
    case 50:
      return VRegister::M0;
    case 51:
      return VRegister::M1;
    case 52:
      return VRegister::M2;
    case 53:
      return VRegister::M3;
    case 54:
      return VRegister::M4;
    case 55:
      return VRegister::M5;
    case 56:
      return VRegister::M6;
    case 57:
      return VRegister::M7;
    case 58:
      return VRegister::M8;
    case 59:
      return VRegister::M9;
    case 60:
      return VRegister::M10;
    case 61:
      return VRegister::M11;
    case 62:
      return VRegister::M12;
    case 63:
      return VRegister::M13;
    case 64:
      return VRegister::M14;
    case 65:
      return VRegister::M15;
    default:
      throw std::runtime_error("Invalid VRegister ID.");
  }
}

bool VRegister::IsVRegister(int id) { return id >= 50 && id <= 65; }

int VRegister::Id() const { return id_; }

VRegister VRegister::M0(x86::ymm0, x86::xmm0, 50);
VRegister VRegister::M1(x86::ymm1, x86::xmm1, 51);
VRegister VRegister::M2(x86::ymm2, x86::xmm2, 52);
VRegister VRegister::M3(x86::ymm3, x86::xmm3, 53);
VRegister VRegister::M4(x86::ymm4, x86::xmm4, 54);
VRegister VRegister::M5(x86::ymm5, x86::xmm5, 55);
VRegister VRegister::M6(x86::ymm6, x86::xmm6, 56);
VRegister VRegister::M7(x86::ymm7, x86::xmm7, 57);
VRegister VRegister::M8(x86::ymm8, x86::xmm8, 58);
VRegister VRegister::M9(x86::ymm9, x86::xmm9, 59);
VRegister VRegister::M10(x86::ymm10, x86::xmm10, 60);
VRegister VRegister::M11(x86::ymm11, x86::xmm11, 61);
VRegister VRegister::M12(x86::ymm12, x86::xmm12, 62);
VRegister VRegister::M13(x86::ymm13, x86::xmm13, 63);
VRegister VRegister::M14(x86::ymm14, x86::xmm14, 64);
VRegister VRegister::M15(x86::ymm15, x86::xmm15, 65);

FRegister::FRegister(int id) : id_(id) {}

int FRegister::Id() { return id_; }

bool FRegister::IsFlag(int id) { return id == 100; }

FRegister FRegister::Flag(100);

}  // namespace kush::khir