#pragma once

#include <functional>

#include "compile/khir/program_builder.h"
#include "compile/proxy/int.h"

namespace kush::compile::proxy {

class TupleIdxTable {
 public:
  TupleIdxTable(khir::ProgramBuilder& program);
  ~TupleIdxTable();

  void Insert(const khir::Value& idx_arr, Int32& num_tables);
  void ForEach(std::function<void(const khir::Value&)> handler);

  static void ForwardDeclare(khir::ProgramBuilder& program);

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
};

}  // namespace kush::compile::proxy