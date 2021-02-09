#include "compile/proxy/function.h"

#include <functional>
#include <utility>
#include <vector>

#include "compile/ir_registry.h"
#include "compile/program_builder.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/struct.h"

namespace kush::compile::proxy {

template <typename T>
ComparisonFunction<T>::ComparisonFunction(
    ProgramBuilder<T>& program, StructBuilder<T> element,
    std::function<void(Struct<T>&, Struct<T>&, std::function<void(Bool<T>)>)>
        body) {
  auto& current_block = program.CurrentBlock();

  auto& type = program.PointerType(element.GenerateType());
  func = &program.CreateFunction(program.I8Type(), {type, type});
  auto args = program.GetFunctionArguments(*func);

  Struct<T> s1(program, element, args[0]);
  Struct<T> s2(program, element, args[1]);
  auto return_func = [&](Bool<T> arg) { program.Return(arg.Get()); };
  body(s1, s2, return_func);

  program.SetCurrentBlock(current_block);
}

template <typename T>
typename ProgramBuilder<T>::Function& ComparisonFunction<T>::Get() {
  return *func;
}

INSTANTIATE_ON_IR(ComparisonFunction);

}  // namespace kush::compile::proxy