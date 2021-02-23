#pragma once

#include "compile/program_builder.h"
#include "compile/proxy/struct.h"

namespace kush::compile::proxy {

template <typename T>
class HashTable {
 public:
  HashTable(ProgramBuilder<T>& program, StructBuilder<T>& content);
  ~HashTable();

  Struct<T> Insert(std::vector<std::reference_wrapper<proxy::Value<T>>> keys);

 private:
  ProgramBuilder<T>& program_;
};

}  // namespace kush::compile::proxy