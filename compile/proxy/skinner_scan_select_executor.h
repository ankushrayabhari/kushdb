#pragma once

#include <functional>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/types/span.h"

#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

class SkinnerScanSelectExecutor {
 public:
  static void ExecutePermutableScanSelect(
      khir::ProgramBuilder& program,
      std::vector<int>& index_executable_predicates,
      khir::Value main_func, khir::Value index_array,
      khir::Value index_array_size, khir::Value predicate_array,
      int num_predicates, khir::Value progress_idx);

  static khir::Value GetFn(khir::ProgramBuilder& program, khir::Value f_arr,
                           proxy::Int32 i);

  static void ForwardDeclare(khir::ProgramBuilder& program);
};

}  // namespace kush::compile::proxy