#include "compile/proxy/skinner_join_executor.h"

#include <functional>
#include <vector>

#include "compile/ir_registry.h"
#include "compile/program_builder.h"
#include "compile/proxy/int.h"

namespace kush::compile::proxy {

template <typename T>
TableFunction<T>::TableFunction(
    ProgramBuilder<T>& program,
    std::function<proxy::Int32<T>(proxy::Int32<T>&, proxy::Int8<T>&)> body) {
  auto current_block = program.CurrentBlock();

  func_ = program.CreateFunction(program.I32Type(),
                                 {program.I32Type(), program.I8Type()});

  auto args = program.GetFunctionArguments(func_.value());
  proxy::Int32<T> budget(program, args[0]);
  proxy::Int8<T> resume_progress(program, args[1]);

  auto x = body(budget, resume_progress);
  program.Return(x.Get());

  program.SetCurrentBlock(current_block);
}

template <typename T>
typename ProgramBuilder<T>::Function TableFunction<T>::Get() {
  return func_.value();
}

INSTANTIATE_ON_IR(TableFunction);

constexpr std::string_view fn =
    "_ZN4kush7runtime18ExecuteSkinnerJoinEiiPPFiiaES2_iPiS4_PaS4_S4_S4_S4_"
    "S4_S4_";

template <typename T>
SkinnerJoinExecutor<T>::SkinnerJoinExecutor(ProgramBuilder<T>& program)
    : program_(program) {}

template <typename T>
void SkinnerJoinExecutor<T>::Execute(
    absl::Span<const typename ProgramBuilder<T>::Value> args) {
  program_.Call(program_.GetFunction(fn), args);
}

template <typename T>
void SkinnerJoinExecutor<T>::ForwardDeclare(ProgramBuilder<T>& program) {
  auto handler_type = program.FunctionType(
      program.I32Type(), {program.I32Type(), program.I8Type()});
  auto handler_pointer_type = program.PointerType(handler_type);

  program.DeclareExternalFunction(fn, program.VoidType(),
                                  {
                                      program.I32Type(),
                                      program.I32Type(),
                                      program.PointerType(handler_pointer_type),
                                      handler_pointer_type,
                                      program.I32Type(),
                                      program.PointerType(program.I32Type()),
                                      program.PointerType(program.I32Type()),
                                      program.PointerType(program.I8Type()),
                                      program.PointerType(program.I32Type()),
                                      program.PointerType(program.I32Type()),
                                      program.PointerType(program.I32Type()),
                                      program.PointerType(program.I32Type()),
                                      program.PointerType(program.I32Type()),
                                      program.PointerType(program.I32Type()),
                                  });
}

INSTANTIATE_ON_IR(SkinnerJoinExecutor);

}  // namespace kush::compile::proxy