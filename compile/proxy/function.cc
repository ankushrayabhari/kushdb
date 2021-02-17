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

  auto& type = program.PointerType(element.Type());
  auto& arg_ptr_type = program.PointerType(program.I8Type());
  func =
      &program.CreateFunction(program.I8Type(), {arg_ptr_type, arg_ptr_type});
  auto args = program.GetFunctionArguments(*func);

  auto& s1_ptr = program.PointerCast(args[0], type);
  auto& s2_ptr = program.PointerCast(args[0], type);

  Struct<T> s1(program, element, s1_ptr);
  Struct<T> s2(program, element, s2_ptr);
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