#include "compile/proxy/hash_table.h"

#include <functional>
#include <vector>

#include "compile/ir_registry.h"
#include "compile/program_builder.h"
#include "compile/proxy/function.h"
#include "compile/proxy/struct.h"

namespace kush::compile::proxy {

template <typename T>
ForwardDeclaredHashTableFunctions<T>::ForwardDeclaredHashTableFunctions(
    typename ProgramBuilder<T>::Type& hash_table_type,
    typename ProgramBuilder<T>::Function& create_func,
    typename ProgramBuilder<T>::Function& get_func,
    typename ProgramBuilder<T>::Function& insert_func,
    typename ProgramBuilder<T>::Function& free_func,
    typename ProgramBuilder<T>::Function& combine_func)
    : hash_table_type_(hash_table_type),
      create_func_(create_func),
      get_func_(get_func),
      insert_func_(insert_func),
      free_func_(free_func),
      combine_func_(combine_func) {}

template <typename T>
typename ProgramBuilder<T>::Type&
ForwardDeclaredHashTableFunctions<T>::HashTableType() {
  return hash_table_type_;
}

template <typename T>
typename ProgramBuilder<T>::Function&
ForwardDeclaredHashTableFunctions<T>::Create() {
  return create_func_;
}

template <typename T>
typename ProgramBuilder<T>::Function&
ForwardDeclaredHashTableFunctions<T>::Get() {
  return get_func_;
}

template <typename T>
typename ProgramBuilder<T>::Function&
ForwardDeclaredHashTableFunctions<T>::Insert() {
  return insert_func_;
}

template <typename T>
typename ProgramBuilder<T>::Function&
ForwardDeclaredHashTableFunctions<T>::Free() {
  return free_func_;
}

template <typename T>
typename ProgramBuilder<T>::Function&
ForwardDeclaredHashTableFunctions<T>::Combine() {
  return combine_func_;
}

INSTANTIATE_ON_IR(ForwardDeclaredHashTableFunctions);

template <typename T>
HashTable<T>::HashTable(ProgramBuilder<T>& program,
                        ForwardDeclaredHashTableFunctions<T> hash_table_funcs,
                        StructBuilder<T>& content)
    : program_(program),
      hash_table_funcs_(hash_table_funcs),
      content_(content),
      content_type_(content_.Type()),
      value_(program_.Alloca(hash_table_funcs.HashTableType())) {}

template <typename T>
HashTable<T>::~HashTable() {
  program_.Call(hash_table_funcs_.Free(), {value_});
}

template <typename T>
Struct<T> HashTable<T>::Insert(
    std::vector<std::reference_wrapper<proxy::Value<T>>> keys) {
  auto& hash_ptr = program_.Alloca(program_.I64Type());
  for (auto& k : keys) {
    auto& k_hash = k.get().Hash();
    program_.Call(hash_table_funcs_.Combine(), {hash_ptr, k_hash});
  }
  auto& hash = program_.Load(hash_ptr);

  auto& data = program_.Call(hash_table_funcs_.Insert(), {value_, hash});
  auto& ptr = program_.PointerCast(data, content_type_);
  return Struct<T>(program_, content_, ptr);
}

template <typename T>
void HashTable<T>::Get(
    std::vector<std::reference_wrapper<proxy::Value<T>>> keys,
    std::function<void(Struct<T>&)> handler) {
  auto& hash_ptr = program_.Alloca(program_.I64Type());
  for (auto& k : keys) {
    auto& k_hash = k.get().Hash();
    program_.Call(hash_table_funcs_.Combine(), {hash_ptr, k_hash});
  }
  auto& hash = program_.Load(hash_ptr);

  proxy::CallbackFunction<T> function(
      program_, content_, [&](Struct<T>& element) { handler(element); });

  program_.Call(hash_table_funcs_.Get(), {value_, hash, function.Get()});
}

INSTANTIATE_ON_IR(HashTable);

}  // namespace kush::compile::proxy