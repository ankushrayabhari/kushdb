#include "compile/codegen/if.h"

#include <functional>

#include "compile/llvm/llvm_program.h"
#include "compile/proxy/boolean.h"

namespace kush::compile::codegen {

If::If(CppProgram& program, proxy::Boolean& boolean,
       std::function<void()> then_fn) {
  program.fout << "if (" << boolean.Get() << ") {";
  then_fn();
  program.fout << "}";
}

If::If(CppProgram& program, proxy::Boolean& boolean,
       std::function<void()> then_fn, std::function<void()> else_fn) {
  program.fout << "if (" << boolean.Get() << ") {";
  then_fn();
  program.fout << "} else {";
  else_fn();
  program.fout << "}";
}

}  // namespace kush::compile::codegen