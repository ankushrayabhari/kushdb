#include "compile/proxy/skinner_scan_select_executor.h"

#include <functional>
#include <vector>

#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"
#include "runtime/skinner_scan_select_executor.h"

namespace kush::compile::proxy {

constexpr std::string_view permutable_scan_select_fn(
    "kush::runtime::ExecutePermutableScanSelect");

void SkinnerScanSelectExecutor::ExecutePermutableScanSelect(
    khir::ProgramBuilder& program, int num_predicates,
    std::vector<int32_t>* indexed_predicates, khir::Value index_scan_fn,
    khir::Value scan_fn, khir::Value num_handlers, khir::Value handlers,
    khir::Value idx) {
  program.Call(
      program.GetFunction(permutable_scan_select_fn),
      {program.ConstI32(num_predicates), program.ConstPtr(indexed_predicates),
       index_scan_fn, scan_fn, num_handlers, handlers, idx});
}

void SkinnerScanSelectExecutor::ForwardDeclare(khir::ProgramBuilder& program) {
  auto scan_type = program.FunctionType(program.I32Type(),
                                        {program.I32Type(), program.I1Type()});
  auto scan_pointer_type = program.PointerType(scan_type);

  auto predicate_type = program.FunctionType(program.I32Type(), {});
  auto predicate_pointer_type = program.PointerType(predicate_type);

  program.DeclareExternalFunction(
      permutable_scan_select_fn, program.VoidType(),
      {
          program.I32Type(),
          program.PointerType(program.I8Type()),
          scan_pointer_type,
          scan_pointer_type,
          program.PointerType(program.I32Type()),
          program.PointerType(predicate_pointer_type),
          program.PointerType(program.I32Type()),
      },
      reinterpret_cast<void*>(&runtime::ExecutePermutableSkinnerScanSelect));
}

}  // namespace kush::compile::proxy