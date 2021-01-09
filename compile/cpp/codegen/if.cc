#include "compile/cpp/codegen/if.h"

#include <functional>

#include "compile/cpp/cpp_program.h"
#include "compile/cpp/proxy/boolean.h"

namespace kush::compile::cpp::codegen {

If::If(CppProgram& program, proxy::Boolean boolean,
       std::function<void(CppProgram&)> then_fn) {
  program.fout << "if (";
  boolean.Get();
  program.fout << ") {";
  then_fn(program);
  program.fout << "}";
}

If::If(CppProgram& program, proxy::Boolean boolean,
       std::function<void(CppProgram&)> then_fn,
       std::function<void(CppProgram&)> else_fn) {
  program.fout << "if (";
  boolean.Get();
  program.fout << ") {";
  then_fn(program);
  program.fout << "} else {";
  else_fn(program);
  program.fout << "}";
}

}  // namespace kush::compile::cpp::codegen