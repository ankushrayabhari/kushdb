#include "compile/proxy/skinner_scan_select_executor.h"

#include <functional>
#include <vector>

#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"
#include "runtime/skinner_scan_select_executor.h"

namespace kush::compile::proxy {

constexpr std::string_view permutable_scan_select_fn(
    "kush::runtime::ExecutePermutableScanSelect");

constexpr std::string_view get_predicate_fn("kush::runtime::GetPredicateFn");

void SkinnerScanSelectExecutor::ExecutePermutableScanSelect(
    khir::ProgramBuilder& program,
    std::vector<int>& index_executable_predicates, khir::Value main_func,
    khir::Value index_array, khir::Value index_array_size,
    khir::Value predicate_array, int num_predicates, khir::Value progress_idx) {
  program.Call(program.GetFunction(permutable_scan_select_fn),
               {program.ConstPtr(&index_executable_predicates), main_func,
                index_array, index_array_size, predicate_array,
                program.ConstI32(num_predicates), progress_idx});
}

khir::Value SkinnerScanSelectExecutor::GetFn(khir::ProgramBuilder& program,
                                             khir::Value f_arr,
                                             proxy::Int32 i) {
  return program.Call(program.GetFunction(get_predicate_fn), {f_arr, i.Get()});
}

void SkinnerScanSelectExecutor::ForwardDeclare(khir::ProgramBuilder& program) {
  auto predicate_type = program.FunctionType(program.I1Type(), {});
  auto predicate_pointer_type = program.PointerType(predicate_type);

  auto main_fn_type = program.FunctionType(
      program.I32Type(), {program.I32Type(), program.I32Type()});
  auto main_fn_ptr_type = program.PointerType(main_fn_type);

  program.DeclareExternalFunction(
      permutable_scan_select_fn, program.VoidType(),
      {
          program.PointerType(program.I8Type()),
          main_fn_ptr_type,
          program.PointerType(program.I32Type()),
          program.PointerType(program.I32Type()),
          program.PointerType(predicate_pointer_type),
          program.I32Type(),
          program.PointerType(program.I32Type()),
      },
      reinterpret_cast<void*>(&runtime::ExecutePermutableSkinnerScanSelect));

  program.DeclareExternalFunction(
      get_predicate_fn, predicate_pointer_type,
      {program.PointerType(predicate_pointer_type), program.I32Type()},
      reinterpret_cast<void*>(&runtime::GetPredicateFn));
}

}  // namespace kush::compile::proxy