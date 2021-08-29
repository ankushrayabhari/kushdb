#include "compile/proxy/skinner_join_executor.h"

#include <functional>
#include <vector>

#include "compile/proxy/int.h"
#include "compile/translators/recompiling_join_translator.h"
#include "khir/program_builder.h"
#include "runtime/skinner_join_executor.h"

namespace kush::compile::proxy {

TableFunction::TableFunction(
    khir::ProgramBuilder& program,
    std::function<proxy::Int32(proxy::Int32&, proxy::Bool&)> body) {
  auto current_block = program.CurrentBlock();

  func_ = program.CreateFunction(program.I32Type(),
                                 {program.I32Type(), program.I1Type()});

  auto args = program.GetFunctionArguments(func_.value());
  proxy::Int32 budget(program, args[0]);
  proxy::Bool resume_progress(program, args[1]);

  auto x = body(budget, resume_progress);
  program.Return(x.Get());

  program.SetCurrentBlock(current_block);
}

khir::FunctionRef TableFunction::Get() { return func_.value(); }

constexpr std::string_view permutable_fn(
    "_ZN4kush7runtime28ExecutePermutableSkinnerJoinEiiPPFiiaES2_iPiS4_PaS4_S4_"
    "S4_S4_S4_S4_");

constexpr std::string_view recompiling_fn(
    "_ZN4kush7runtime29ExecuteRecompilingSkinnerJoinEiiPiS1_PNS_"
    "7compile25RecompilingJoinTranslatorEPPvS6_S5_");

SkinnerJoinExecutor::SkinnerJoinExecutor(khir::ProgramBuilder& program)
    : program_(program) {}

void SkinnerJoinExecutor::ExecutePermutableJoin(
    absl::Span<const khir::Value> args) {
  program_.Call(program_.GetFunction(permutable_fn), args);
}

void SkinnerJoinExecutor::ExecuteRecompilingJoin(
    int32_t num_tables, int32_t num_predicates, khir::Value cardinality_arr,
    khir::Value tables_per_predicate, RecompilingJoinTranslator* obj,
    khir::Value materialized_buffers, khir::Value materialized_indexes,
    khir::Value tuple_idx_table) {
  program_.Call(
      program_.GetFunction(recompiling_fn),
      {program_.ConstI32(num_tables), program_.ConstI32(num_predicates),
       cardinality_arr, tables_per_predicate,
       program_.ConstPtr(static_cast<void*>(obj)), materialized_buffers,
       materialized_indexes, tuple_idx_table});
}

void SkinnerJoinExecutor::ForwardDeclare(khir::ProgramBuilder& program) {
  auto handler_type = program.FunctionType(
      program.I32Type(), {program.I32Type(), program.I1Type()});
  auto handler_pointer_type = program.PointerType(handler_type);

  program.DeclareExternalFunction(
      permutable_fn, program.VoidType(),
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
      },
      reinterpret_cast<void*>(&runtime::ExecutePermutableSkinnerJoin));

  program.DeclareExternalFunction(
      recompiling_fn, program.VoidType(),
      {
          program.I32Type(),
          program.I32Type(),
          program.PointerType(program.I32Type()),
          program.PointerType(program.I32Type()),
          program.PointerType(program.I8Type()),
          program.PointerType(program.PointerType(program.I8Type())),
          program.PointerType(program.PointerType(program.I8Type())),
          program.PointerType(program.I8Type()),
      },
      reinterpret_cast<void*>(&runtime::ExecuteRecompilingSkinnerJoin));
}

}  // namespace kush::compile::proxy