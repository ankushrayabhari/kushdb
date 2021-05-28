#include "compile/proxy/skinner_join_executor.h"

#include <functional>
#include <vector>

#include "compile/khir/khir_program_builder.h"
#include "compile/proxy/int.h"

namespace kush::compile::proxy {

TableFunction::TableFunction(
    khir::KHIRProgramBuilder& program,
    std::function<proxy::Int32(proxy::Int32&, proxy::Int8&)> body) {
  auto current_block = program.CurrentBlock();

  func_ = program.CreateFunction(program.I32Type(),
                                 {program.I32Type(), program.I8Type()});

  auto args = program.GetFunctionArguments(func_.value());
  proxy::Int32 budget(program, args[0]);
  proxy::Int8 resume_progress(program, args[1]);

  auto x = body(budget, resume_progress);
  program.Return(x.Get());

  program.SetCurrentBlock(current_block);
}

khir::FunctionRef TableFunction::Get() { return func_.value(); }

constexpr std::string_view fn =
    "_ZN4kush7runtime18ExecuteSkinnerJoinEiiPPFiiaES2_iPiS4_PaS4_S4_S4_S4_"
    "S4_S4_";

SkinnerJoinExecutor::SkinnerJoinExecutor(khir::KHIRProgramBuilder& program)
    : program_(program) {}

void SkinnerJoinExecutor::Execute(absl::Span<const khir::Value> args) {
  program_.Call(program_.GetFunction(fn), args);
}

void SkinnerJoinExecutor::ForwardDeclare(khir::KHIRProgramBuilder& program) {
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

}  // namespace kush::compile::proxy