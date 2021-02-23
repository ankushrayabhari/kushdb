#pragma once

#include <functional>
#include <vector>

#include "compile/program_builder.h"
#include "compile/proxy/struct.h"
#include "compile/proxy/vector.h"

namespace kush::compile::proxy {

template <typename T>
class ForwardDeclaredHashTableFunctions {
 public:
  ForwardDeclaredHashTableFunctions(
      typename ProgramBuilder<T>::Type& hash_table_type,
      typename ProgramBuilder<T>::Function& create_func,
      typename ProgramBuilder<T>::Function& insert_func,
      typename ProgramBuilder<T>::Function& get_bucket_func_,
      typename ProgramBuilder<T>::Function& free_func,
      typename ProgramBuilder<T>::Function& combine_func);

  typename ProgramBuilder<T>::Type& HashTableType();
  typename ProgramBuilder<T>::Function& Create();
  typename ProgramBuilder<T>::Function& GetBucket();
  typename ProgramBuilder<T>::Function& Insert();
  typename ProgramBuilder<T>::Function& Free();
  typename ProgramBuilder<T>::Function& Combine();

 private:
  typename ProgramBuilder<T>::Type& hash_table_type_;
  typename ProgramBuilder<T>::Function& create_func_;
  typename ProgramBuilder<T>::Function& get_bucket_func_;
  typename ProgramBuilder<T>::Function& insert_func_;
  typename ProgramBuilder<T>::Function& free_func_;
  typename ProgramBuilder<T>::Function& combine_func_;
};

template <typename T>
class HashTable {
 public:
  HashTable(ProgramBuilder<T>& program,
            ForwardDeclaredVectorFunctions<T>& vector_funcs,
            ForwardDeclaredHashTableFunctions<T>& hash_table_funcs,
            StructBuilder<T>& content);
  ~HashTable();

  Struct<T> Insert(std::vector<std::reference_wrapper<proxy::Value<T>>> keys);
  void Get(std::vector<std::reference_wrapper<proxy::Value<T>>> keys,
           std::function<void(Struct<T>&)> handler);
  void ForEach(std::function<void(Struct<T>&)> handler);

  static ForwardDeclaredHashTableFunctions<T> ForwardDeclare(
      ProgramBuilder<T>& program,
      ForwardDeclaredVectorFunctions<T>& vector_funcs);

 private:
  ProgramBuilder<T>& program_;
  ForwardDeclaredVectorFunctions<T>& vector_funcs_;
  ForwardDeclaredHashTableFunctions<T>& hash_table_funcs_;
  StructBuilder<T>& content_;
  typename ProgramBuilder<T>::Type& content_type_;
  typename ProgramBuilder<T>::Value& value_;
  typename ProgramBuilder<T>::Value& hash_ptr_;
};

}  // namespace kush::compile::proxy