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
      khir::ProgramBuilder& program, int num_predicates,
      std::vector<int32_t>* indexed_predicates, khir::Value index_scan_fn,
      khir::Value scan_fn, khir::Value num_handlers, khir::Value handlers,
      khir::Value idx);

  static void ForwardDeclare(khir::ProgramBuilder& program);
};

}  // namespace kush::compile::proxy