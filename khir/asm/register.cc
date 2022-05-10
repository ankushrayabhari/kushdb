#include "khir/asm/register.h"

#include "asmjit/x86.h"

namespace kush::khir {

using namespace asmjit;

GPRegister::GPRegister(asmjit::x86::Gpq qword, asmjit::x86::Gpd dword,
                       asmjit::x86::Gpw word, asmjit::x86::GpbLo byt)
    : qword_(qword), dword_(dword), word_(word), byte_(byt) {}

asmjit::x86::Gpq GPRegister::GetQ() const { return qword_; }

asmjit::x86::Gpd GPRegister::GetD() const { return dword_; }

asmjit::x86::Gpw GPRegister::GetW() const { return word_; }

asmjit::x86::GpbLo GPRegister::GetB() const { return byte_; }

GPRegister GPRegister::RAX(x86::rax, x86::eax, x86::ax, x86::al);
GPRegister GPRegister::RBX(x86::rbx, x86::ebx, x86::bx, x86::bl);
GPRegister GPRegister::RCX(x86::rcx, x86::ecx, x86::cx, x86::cl);
GPRegister GPRegister::RDX(x86::rdx, x86::edx, x86::dx, x86::dl);
GPRegister GPRegister::RSI(x86::rsi, x86::esi, x86::si, x86::sil);
GPRegister GPRegister::RDI(x86::rdi, x86::edi, x86::di, x86::dil);
GPRegister GPRegister::RSP(x86::rsp, x86::esp, x86::sp, x86::spl);
GPRegister GPRegister::RBP(x86::rbp, x86::ebp, x86::bp, x86::bpl);
GPRegister GPRegister::R8(x86::r8, x86::r8d, x86::r8w, x86::r8b);
GPRegister GPRegister::R9(x86::r9, x86::r9d, x86::r9w, x86::r9b);
GPRegister GPRegister::R10(x86::r10, x86::r10d, x86::r10w, x86::r10b);
GPRegister GPRegister::R11(x86::r11, x86::r11d, x86::r11w, x86::r11b);
GPRegister GPRegister::R12(x86::r12, x86::r12d, x86::r12w, x86::r12b);
GPRegister GPRegister::R13(x86::r13, x86::r13d, x86::r13w, x86::r13b);
GPRegister GPRegister::R14(x86::r14, x86::r14d, x86::r14w, x86::r14b);
GPRegister GPRegister::R15(x86::r15, x86::r15d, x86::r15w, x86::r15b);

VRegister::VRegister(asmjit::x86::Ymm ymm, asmjit::x86::Xmm xmm)
    : ymm_(ymm), xmm_(xmm) {}

asmjit::x86::Xmm VRegister::GetX() const { return xmm_; }

asmjit::x86::Ymm VRegister::GetY() const { return ymm_; }

VRegister VRegister::M0(x86::ymm0, x86::xmm0);
VRegister VRegister::M1(x86::ymm1, x86::xmm1);
VRegister VRegister::M2(x86::ymm2, x86::xmm2);
VRegister VRegister::M3(x86::ymm3, x86::xmm3);
VRegister VRegister::M4(x86::ymm4, x86::xmm4);
VRegister VRegister::M5(x86::ymm5, x86::xmm5);
VRegister VRegister::M6(x86::ymm6, x86::xmm6);
VRegister VRegister::M7(x86::ymm7, x86::xmm7);
VRegister VRegister::M8(x86::ymm8, x86::xmm8);
VRegister VRegister::M9(x86::ymm9, x86::xmm9);
VRegister VRegister::M10(x86::ymm10, x86::xmm10);
VRegister VRegister::M11(x86::ymm11, x86::xmm11);
VRegister VRegister::M12(x86::ymm12, x86::xmm12);
VRegister VRegister::M13(x86::ymm13, x86::xmm13);
VRegister VRegister::M14(x86::ymm14, x86::xmm14);
VRegister VRegister::M15(x86::ymm15, x86::xmm15);

}  // namespace kush::khir