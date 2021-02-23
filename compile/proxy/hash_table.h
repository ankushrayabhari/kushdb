#pragma once

#include <functional>
#include <vector>

#include "compile/program_builder.h"
#include "compile/proxy/struct.h"

namespace kush::compile::proxy {

template <typename T>
class HashTable {
 public:
  HashTable(ProgramBuilder<T>& program, StructBuilder<T>& content);
  ~HashTable();

  Struct<T> Insert(std::vector<std::reference_wrapper<proxy::Value<T>>> keys);
  void Get(std::vector<std::reference_wrapper<proxy::Value<T>>> keys,
           std::function<void(Struct<T>&)> handler);

 private:
  ProgramBuilder<T>& program_;
};

}  // namespace kush::compile::proxy