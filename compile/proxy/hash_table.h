#pragma once

#include <functional>
#include <vector>

#include "compile/program_builder.h"
#include "compile/proxy/struct.h"
#include "compile/proxy/vector.h"

namespace kush::compile::proxy {

template <typename T>
class HashTable {
 public:
  HashTable(ProgramBuilder<T>& program, StructBuilder<T>& content);
  ~HashTable();

  Struct<T> Insert(std::vector<std::reference_wrapper<Value<T>>> keys);
  Vector<T> Get(std::vector<std::reference_wrapper<Value<T>>> keys);
  void ForEach(std::function<void(Struct<T>&)> handler);

  static void ForwardDeclare(ProgramBuilder<T>& program);

 private:
  ProgramBuilder<T>& program_;
  StructBuilder<T>& content_;
  typename ProgramBuilder<T>::Type content_type_;
  typename ProgramBuilder<T>::Value value_;
  typename ProgramBuilder<T>::Value hash_ptr_;
  typename ProgramBuilder<T>::Value bucket_list_;
};

}  // namespace kush::compile::proxy