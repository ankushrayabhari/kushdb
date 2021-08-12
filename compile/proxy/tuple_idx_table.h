#pragma once

#include <functional>

#include "compile/proxy/int.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

class TupleIdxTable {
 public:
  TupleIdxTable(khir::ProgramBuilder& program, bool global);
  TupleIdxTable(khir::ProgramBuilder& program, khir::Value value);

  void Reset();
  khir::Value Get();

  void Insert(const khir::Value& idx_arr, const Int32& num_tables);
  void ForEach(std::function<void(const khir::Value&)> handler);
  static void ForwardDeclare(khir::ProgramBuilder& program);

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
};

}  // namespace kush::compile::proxy