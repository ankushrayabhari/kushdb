#include "khir/asm/register.h"

#include "asmjit/x86.h"

namespace kush::khir {

using namespace asmjit;

Register::Register(asmjit::x86::Gpq qword, asmjit::x86::Gpd dword,
                   asmjit::x86::Gpw word, asmjit::x86::Gpb byt)
    : qword_(qword), dword_(dword), word_(word), byte_(byt) {}

asmjit::x86::Gpq Register::GetQ() { return qword_; }

asmjit::x86::Gpd Register::GetD() { return dword_; }

asmjit::x86::Gpw Register::GetW() { return word_; }

asmjit::x86::Gpb Register::GetB() { return byte_; }

Register Register::RAX(x86::rax, x86::eax, x86::ax, x86::al);
Register Register::RBX(x86::rbx, x86::ebx, x86::bx, x86::bl);
Register Register::RCX(x86::rcx, x86::ecx, x86::cx, x86::cl);
Register Register::RDX(x86::rdx, x86::edx, x86::dx, x86::dl);
Register Register::RSI(x86::rsi, x86::esi, x86::si, x86::sil);
Register Register::RDI(x86::rdi, x86::edi, x86::di, x86::dil);
Register Register::RSP(x86::rsp, x86::esp, x86::sp, x86::spl);
Register Register::RBP(x86::rbp, x86::ebp, x86::bp, x86::bpl);
Register Register::R8(x86::r8, x86::r8d, x86::r8w, x86::r8b);
Register Register::R9(x86::r9, x86::r9d, x86::r9w, x86::r9b);
Register Register::R10(x86::r10, x86::r10d, x86::r10w, x86::r10b);
Register Register::R11(x86::r11, x86::r11d, x86::r11w, x86::r11b);
Register Register::R12(x86::r12, x86::r12d, x86::r12w, x86::r12b);
Register Register::R13(x86::r13, x86::r13d, x86::r13w, x86::r13b);
Register Register::R14(x86::r14, x86::r14d, x86::r14w, x86::r14b);
Register Register::R15(x86::r15, x86::r15d, x86::r15w, x86::r15b);

}  // namespace kush::khir