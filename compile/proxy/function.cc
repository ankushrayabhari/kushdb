#include "compile/proxy/function.h"

#include <functional>
#include <utility>
#include <vector>

#include "compile/proxy/struct.h"
#include "compile/proxy/value/value.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

ComparisonFunction::ComparisonFunction(
    khir::ProgramBuilder& program, StructBuilder element,
    std::function<void(Struct&, Struct&, std::function<void(Bool)>)> body) {
  auto current_block = program.CurrentBlock();

  auto type = program.PointerType(element.Type());
  auto arg_ptr_type = program.PointerType(program.I8Type());
  func = program.CreateFunction(program.I1Type(), {arg_ptr_type, arg_ptr_type});
  auto args = program.GetFunctionArguments(func.value());

  auto s1_ptr = program.PointerCast(args[0], type);
  auto s2_ptr = program.PointerCast(args[1], type);

  Struct s1(program, element, s1_ptr);
  Struct s2(program, element, s2_ptr);
  auto return_func = [&](Bool arg) { program.Return(arg.Get()); };
  body(s1, s2, return_func);

  program.SetCurrentBlock(current_block);
}

khir::FunctionRef ComparisonFunction::Get() { return func.value(); }

}  // namespace kush::compile::proxy