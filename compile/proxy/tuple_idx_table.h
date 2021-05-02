#pragma once

#include <functional>

#include "compile/program_builder.h"
#include "compile/proxy/int.h"

namespace kush::compile::proxy {

template <typename T>
class TupleIdxTable {
 public:
  TupleIdxTable(ProgramBuilder<T>& program);
  ~TupleIdxTable();

  void Insert(const typename ProgramBuilder<T>::Value& idx_arr,
              Int32<T>& num_tables);
  void ForEach(
      std::function<void(const typename ProgramBuilder<T>::Value&)> handler);

  static void ForwardDeclare(ProgramBuilder<T>& program);

 private:
  ProgramBuilder<T>& program_;
  typename ProgramBuilder<T>::Value value_;
};

}  // namespace kush::compile::proxy