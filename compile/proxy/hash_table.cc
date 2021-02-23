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
    typename ProgramBuilder<T>::Type& bucket_list_type,
    typename ProgramBuilder<T>::Function& create_func,
    typename ProgramBuilder<T>::Function& insert_func,
    typename ProgramBuilder<T>::Function& get_bucket_func,
    typename ProgramBuilder<T>::Function& get_all_bucket_func,
    typename ProgramBuilder<T>::Function& free_func,
    typename ProgramBuilder<T>::Function& combine_func,
    typename ProgramBuilder<T>::Function& size_fn,
    typename ProgramBuilder<T>::Function& get_bucket_idx_fn,
    typename ProgramBuilder<T>::Function& free_buck_fn)
    : hash_table_type_(hash_table_type),
      bucket_list_type_(bucket_list_type),
      create_func_(create_func),
      get_bucket_func_(get_bucket_func),
      get_all_bucket_func_(get_all_bucket_func),
      insert_func_(insert_func),
      free_func_(free_func),
      combine_func_(combine_func),
      size_fn_(size_fn),
      get_bucket_idx_fn_(get_bucket_idx_fn),
      free_buck_fn_(free_buck_fn) {}

template <typename T>
typename ProgramBuilder<T>::Type&
ForwardDeclaredHashTableFunctions<T>::HashTableType() {
  return hash_table_type_;
}

template <typename T>
typename ProgramBuilder<T>::Type&
ForwardDeclaredHashTableFunctions<T>::BucketListType() {
  return bucket_list_type_;
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
ForwardDeclaredHashTableFunctions<T>::GetAllBuckets() {
  return get_all_bucket_func_;
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

template <typename T>
typename ProgramBuilder<T>::Function&
ForwardDeclaredHashTableFunctions<T>::Size() {
  return size_fn_;
}

template <typename T>
typename ProgramBuilder<T>::Function&
ForwardDeclaredHashTableFunctions<T>::GetBucketIdx() {
  return get_bucket_idx_fn_;
}

template <typename T>
typename ProgramBuilder<T>::Function&
ForwardDeclaredHashTableFunctions<T>::FreeBucketList() {
  return free_buck_fn_;
}

INSTANTIATE_ON_IR(ForwardDeclaredHashTableFunctions);

template <typename T>
Bucket<T>::Bucket(ProgramBuilder<T>& program,
                  ForwardDeclaredVectorFunctions<T>& vector_funcs,
                  ForwardDeclaredHashTableFunctions<T>& hash_table_funcs,
                  StructBuilder<T>& content,
                  typename ProgramBuilder<T>::Value& value)
    : program_(program),
      vector_funcs_(vector_funcs),
      hash_table_funcs_(hash_table_funcs),
      content_(content),
      value_(value) {}

template <typename T>
Struct<T> Bucket<T>::Get(UInt32<T> i) {
  auto& data = program_.Call(vector_funcs_.Get(), {value_, i.Get()});
  auto& ptr = program_.PointerCast(data, program_.PointerType(content_.Type()));
  return Struct<T>(program_, content_, ptr);
}

template <typename T>
UInt32<T> Bucket<T>::Size() {
  return proxy::UInt32<T>(program_,
                          program_.Call(vector_funcs_.Size(), {value_}));
}

INSTANTIATE_ON_IR(Bucket);

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
      hash_ptr_(program_.Alloca(program_.I32Type())),
      bucket_list_(program_.Alloca(hash_table_funcs_.BucketListType())) {
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
Bucket<T> HashTable<T>::Get(
    std::vector<std::reference_wrapper<proxy::Value<T>>> keys) {
  program_.Store(hash_ptr_, program_.ConstUI32(0));
  for (auto& k : keys) {
    auto& k_hash = k.get().Hash();
    program_.Call(hash_table_funcs_.Combine(), {hash_ptr_, k_hash});
  }
  auto& hash = program_.Load(hash_ptr_);

  auto& bucket_ptr =
      program_.Call(hash_table_funcs_.GetBucket(), {value_, hash});
  return Bucket<T>(program_, vector_funcs_, hash_table_funcs_, content_,
                   bucket_ptr);
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

  auto& bucket_list_struct = program.StructType(
      {program.I32Type(),
       program.PointerType(program.PointerType(vector_funcs.VectorType()))});
  auto& bucket_list_struct_ptr = program.PointerType(bucket_list_struct);

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

  auto& get_all_bucks_fn = program.DeclareExternalFunction(
      "_ZN4kush4data13GetAllBucketsEPNS0_9HashTableEPNS0_10BucketListE",
      program.VoidType(), {struct_ptr, bucket_list_struct_ptr});

  auto& size_fn = program.DeclareExternalFunction(
      "_ZN4kush4data4SizeEPNS0_10BucketListE", program.UI32Type(),
      {bucket_list_struct_ptr});

  auto& get_buck_idx_fn = program.DeclareExternalFunction(
      "_ZN4kush4data12GetBucketIdxEPNS0_10BucketListEj",
      program.PointerType(vector_funcs.VectorType()),
      {bucket_list_struct_ptr, program.UI32Type()});

  auto& free_buck_fn = program.DeclareExternalFunction(
      "_ZN4kush4data4FreeEPNS0_10BucketListE", program.VoidType(),
      {bucket_list_struct_ptr});

  return ForwardDeclaredHashTableFunctions<T>(
      struct_type, bucket_list_struct, create_fn, insert_fn, get_bucket,
      get_all_bucks_fn, free_fn, combine_fn, size_fn, get_buck_idx_fn,
      free_buck_fn);
}

template <typename T>
void HashTable<T>::ForEach(std::function<void(Struct<T>&)> handler) {
  program_.Call(hash_table_funcs_.GetAllBuckets(), {value_, bucket_list_});

  auto size = proxy::UInt32<T>(
      program_, program_.Call(hash_table_funcs_.Size(), {bucket_list_}));

  proxy::IndexLoop<T>(
      program_, [&]() { return proxy::UInt32<T>(program_, 0); },
      [&](proxy::UInt32<T>& i) { return i < size; },
      [&](proxy::UInt32<T>& i) {
        auto& bucket = program_.Call(hash_table_funcs_.GetBucketIdx(),
                                     {bucket_list_, i.Get()});

        auto bucket_size = proxy::UInt32<T>(
            program_, program_.Call(vector_funcs_.Size(), {bucket}));

        proxy::IndexLoop<T>(
            program_, [&]() { return proxy::UInt32<T>(program_, 0); },
            [&](proxy::UInt32<T>& j) { return j < bucket_size; },
            [&](proxy::UInt32<T>& j) {
              auto& ptr = program_.Call(vector_funcs_.Get(), {bucket, j.Get()});
              auto& struct_ptr = program_.PointerCast(
                  ptr, program_.PointerType(content_type_));
              Struct<T> data(program_, content_, struct_ptr);

              handler(data);

              return j + proxy::UInt32<T>(program_, 1);
            });

        return i + proxy::UInt32<T>(program_, 1);
      });

  program_.Call(hash_table_funcs_.FreeBucketList(), {bucket_list_});
}

INSTANTIATE_ON_IR(HashTable);

}  // namespace kush::compile::proxy