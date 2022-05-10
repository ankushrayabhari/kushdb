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

}  // namespace kush::khir