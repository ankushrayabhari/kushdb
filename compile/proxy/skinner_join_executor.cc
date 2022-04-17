#include "compile/proxy/skinner_join_executor.h"

#include <functional>
#include <vector>

#include "compile/proxy/tuple_idx_table.h"
#include "compile/proxy/value/ir_value.h"
#include "compile/translators/recompiling_join_translator.h"
#include "khir/program_builder.h"
#include "runtime/skinner_join_executor.h"

namespace kush::compile::proxy {

namespace {
constexpr std::string_view permutable_fn(
    "kush::runtime::ExecutePermutableSkinnerJoin");
constexpr std::string_view recompiling_fn(
    "kush::runtime::ExecuteRecompilingSkinnerJoin");
}  // namespace

void SkinnerJoinExecutor::ExecutePermutableJoin(
    khir::ProgramBuilder& program, int32_t num_tables, int32_t num_preds,
    absl::flat_hash_map<std::pair<int, int>, int>* pred_table_to_flag,
    absl::flat_hash_set<std::pair<int, int>>* table_connections,
    const std::vector<int>* prefix_order, khir::Value join_handler_fn_arr,
    khir::Value valid_tuple_handler, int32_t num_flags, khir::Value flag_arr,
    khir::Value progress_arr, khir::Value table_ctr, khir::Value idx_arr,
    khir::Value num_result_tuples, khir::Value offset_arr) {
  program.Call(
      program.GetFunction(permutable_fn),
      {program.ConstI32(num_tables), program.ConstI32(num_preds),
       program.ConstPtr(pred_table_to_flag),
       program.ConstPtr(table_connections),
       program.ConstPtr((void*)prefix_order), join_handler_fn_arr,
       valid_tuple_handler, program.ConstI32(num_flags), flag_arr, progress_arr,
       table_ctr, idx_arr, num_result_tuples, offset_arr});
}

void SkinnerJoinExecutor::ExecuteRecompilingJoin(
    khir::ProgramBuilder& program, int32_t num_tables,
    khir::Value cardinality_arr,
    absl::flat_hash_set<std::pair<int, int>>* table_connections,
    const std::vector<int>* prefix_order, RecompilingJoinTranslator* obj,
    khir::Value materialized_buffers, khir::Value materialized_indexes,
    khir::Value tuple_idx_table) {
  program.Call(program.GetFunction(recompiling_fn),
               {program.ConstI32(num_tables), cardinality_arr,
                program.ConstPtr(table_connections),
                program.ConstPtr((void*)prefix_order), program.ConstPtr(obj),
                materialized_buffers, materialized_indexes, tuple_idx_table});
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
          program.PointerType(program.I8Type()),
          program.PointerType(program.I8Type()),
          program.PointerType(program.I8Type()),
          program.PointerType(handler_pointer_type),
          handler_pointer_type,
          program.I32Type(),
          program.PointerType(program.I8Type()),
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
          program.PointerType(program.I32Type()),
          program.PointerType(program.I8Type()),
          program.PointerType(program.I8Type()),
          program.PointerType(program.I8Type()),
          program.PointerType(program.PointerType(program.I8Type())),
          program.PointerType(program.PointerType(program.I8Type())),
          program.PointerType(program.GetOpaqueType(TupleIdxTable::TypeName)),
      },
      reinterpret_cast<void*>(&runtime::ExecuteRecompilingSkinnerJoin));
}

}  // namespace kush::compile::proxy