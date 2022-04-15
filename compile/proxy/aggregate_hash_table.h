#pragma once

#include <functional>
#include <utility>
#include <vector>

#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

class AggregateHashTableEntry {
 public:
  AggregateHashTableEntry(khir::ProgramBuilder& program, khir::Value v);

  struct Parts {
    Int16 salt;
    Int16 block_offset;
    Int32 block_idx;
  };
  Parts Get();

  void Set(Int16 salt, Int16 offset, Int32 idx);

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
};

}  // namespace kush::compile::proxy