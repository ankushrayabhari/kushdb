#include "compile/proxy/hash_table.h"

#include <functional>
#include <vector>

#include "compile/ir_registry.h"
#include "compile/program_builder.h"
#include "compile/proxy/loop.h"
#include "compile/proxy/struct.h"

namespace kush::compile::proxy {

template <typename T>
ForwardDeclaredHashTableFunctions<T>::ForwardDeclaredHashTableFunctions(
    typename ProgramBuilder<T>::Type& hash_table_type,
    typename ProgramBuilder<T>::Function& create_func,
    typename ProgramBuilder<T>::Function& insert_func,
    typename ProgramBuilder<T>::Function& get_bucket_func,
    typename ProgramBuilder<T>::Function& free_func,
    typename ProgramBuilder<T>::Function& combine_func)
    : hash_table_type_(hash_table_type),
      create_func_(create_func),
      get_bucket_func_(get_bucket_func),
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
ForwardDeclaredHashTableFunctions<T>::GetBucket() {
  return get_bucket_func_;
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
                        ForwardDeclaredVectorFunctions<T>& vector_funcs,
                        ForwardDeclaredHashTableFunctions<T>& hash_table_funcs,
                        StructBuilder<T>& content)
    : program_(program),
      vector_funcs_(vector_funcs),
      hash_table_funcs_(hash_table_funcs),
      content_(content),
      content_type_(content_.Type()),
      value_(program_.Alloca(hash_table_funcs.HashTableType())),
      hash_ptr_(program_.Alloca(program_.I32Type())) {
  auto& element_size = program_.SizeOf(content_type_);
  program_.Call(hash_table_funcs_.Create(), {value_, element_size});
}

template <typename T>
HashTable<T>::~HashTable() {
  program_.Call(hash_table_funcs_.Free(), {value_});
}

template <typename T>
Struct<T> HashTable<T>::Insert(
    std::vector<std::reference_wrapper<proxy::Value<T>>> keys) {
  program_.Store(hash_ptr_, program_.ConstUI32(0));
  for (auto& k : keys) {
    auto& k_hash = k.get().Hash();
    program_.Call(hash_table_funcs_.Combine(), {hash_ptr_, k_hash});
  }
  auto& hash = program_.Load(hash_ptr_);

  auto& data = program_.Call(hash_table_funcs_.Insert(), {value_, hash});
  auto& ptr = program_.PointerCast(data, program_.PointerType(content_type_));
  return Struct<T>(program_, content_, ptr);
}

template <typename T>
void HashTable<T>::Get(
    std::vector<std::reference_wrapper<proxy::Value<T>>> keys,
    std::function<void(Struct<T>&)> handler) {
  program_.Store(hash_ptr_, program_.ConstUI32(0));
  for (auto& k : keys) {
    auto& k_hash = k.get().Hash();
    program_.Call(hash_table_funcs_.Combine(), {hash_ptr_, k_hash});
  }
  auto& hash = program_.Load(hash_ptr_);

  auto& bucket = program_.Call(hash_table_funcs_.GetBucket(), {value_, hash});
  auto length =
      proxy::UInt32<T>(program_, program_.Call(vector_funcs_.Size(), {
                                                                         bucket,
                                                                     }));

  proxy::IndexLoop<T>(
      program_, [&]() { return proxy::UInt32<T>(program_, 0); },
      [&](proxy::UInt32<T>& i) { return i < length; },
      [&](proxy::UInt32<T>& i) {
        auto& ptr = program_.Call(vector_funcs_.Get(), {bucket, i.Get()});
        auto& casted_ptr =
            program_.PointerCast(ptr, program_.PointerType(content_type_));
        Struct<T> element(program_, content_, casted_ptr);
        handler(element);
        return i + proxy::UInt32<T>(program_, 1);
      });
}

template <typename T>
ForwardDeclaredHashTableFunctions<T> HashTable<T>::ForwardDeclare(
    ProgramBuilder<T>& program,
    ForwardDeclaredVectorFunctions<T>& vector_funcs) {
  auto& struct_type = program.StructType({
      program.I64Type(),
      program.PointerType(program.I8Type()),
  });
  auto& struct_ptr = program.PointerType(struct_type);

  auto& create_fn = program.DeclareExternalFunction(
      "_ZN4kush4data6CreateEPNS0_9HashTableEm", program.VoidType(),
      {struct_ptr, program.I64Type()});

  auto& insert_fn = program.DeclareExternalFunction(
      "_ZN4kush4data6InsertEPNS0_9HashTableEj",
      program.PointerType(program.I8Type()), {struct_ptr, program.I32Type()});

  auto& get_bucket = program.DeclareExternalFunction(
      "_ZN4kush4data9GetBucketEPNS0_9HashTableEj",
      program.PointerType(vector_funcs.VectorType()),
      {struct_ptr, program.I32Type()});

  auto& free_fn = program.DeclareExternalFunction(
      "_ZN4kush4data4FreeEPNS0_9HashTableE", program.VoidType(), {struct_ptr});

  auto& combine_fn = program.DeclareExternalFunction(
      "_ZN4kush4data11HashCombineEPjl", program.VoidType(),
      {program.PointerType(program.I32Type()), program.I64Type()});

  return ForwardDeclaredHashTableFunctions<T>(struct_type, create_fn, insert_fn,
                                              get_bucket, free_fn, combine_fn);
}

INSTANTIATE_ON_IR(HashTable);

}  // namespace kush::compile::proxy