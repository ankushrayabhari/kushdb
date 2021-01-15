#include "compile/codegen/loop.h"

#include <functional>

#include "compile/codegen/if.h"
#include "compile/llvm/llvm_program.h"

namespace kush::compile::codegen {

Loop::Loop(CppProgram& program, std::function<void()> init,
           std::function<proxy::Boolean()> cond, std::function<void()> body) {
  init();
  program.fout << "while (true) {";

  auto c = cond();
  If br(program, c, [&program]() { program.fout << "break;"; });

  body();

  program.fout << "}";
}

}  // namespace kush::compile::codegen