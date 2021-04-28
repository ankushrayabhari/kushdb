#include "compile/proxy/hash_table.h"

#include <functional>
#include <vector>

#include "compile/ir_registry.h"
#include "compile/program_builder.h"
#include "compile/proxy/loop.h"
#include "compile/proxy/struct.h"
#include "compile/proxy/vector.h"
#include "util/vector_util.h"

namespace kush::compile::proxy {

template <typename T>
const std::string_view BucketList<T>::BucketListStructName(
    "kush::data::BucketList");

template <typename T>
const std::string_view BucketList<T>::SizeFnName(
    "_ZN4kush4data4SizeEPNS0_10BucketListE");

template <typename T>
const std::string_view BucketList<T>::FreeFnName(
    "_ZN4kush4data4FreeEPNS0_10BucketListE");

template <typename T>
const std::string_view BucketList<T>::GetBucketIdxFnName(
    "_ZN4kush4data12GetBucketIdxEPNS0_10BucketListEi");

template <typename T>
BucketList<T>::BucketList(ProgramBuilder<T>& program, StructBuilder<T>& content,
                          typename ProgramBuilder<T>::Value& value)
    : program_(program), content_(content), value_(value) {}

template <typename T>
Vector<T> BucketList<T>::operator[](const Int32<T>& i) {
  auto& ptr = program_.Call(program_.GetFunction(GetBucketIdxFnName),
                            {value_, i.Get()});
  return Vector<T>(program_, content_, ptr);
}

template <typename T>
Int32<T> BucketList<T>::Size() {
  return proxy::Int32<T>(
      program_, program_.Call(program_.GetFunction(SizeFnName), {value_}));
}

template <typename T>
void BucketList<T>::Reset() {
  program_.Call(program_.GetFunction(FreeFnName), {value_});
}

template <typename T>
void BucketList<T>::ForwardDeclare(ProgramBuilder<T>& program) {
  auto& vector_ptr_type =
      program.PointerType(program.GetStructType(Vector<T>::VectorStructName));

  auto& bucket_list_struct = program.StructType(
      {program.I32Type(), program.PointerType(vector_ptr_type)},
      BucketListStructName);
  auto& bucket_list_struct_ptr = program.PointerType(bucket_list_struct);

  program.DeclareExternalFunction(BucketList<T>::GetBucketIdxFnName,
                                  vector_ptr_type,
                                  {bucket_list_struct_ptr, program.I32Type()});
  program.DeclareExternalFunction(BucketList<T>::SizeFnName, program.I32Type(),
                                  {bucket_list_struct_ptr});
  program.DeclareExternalFunction(BucketList<T>::FreeFnName, program.VoidType(),
                                  {bucket_list_struct_ptr});
}

INSTANTIATE_ON_IR(BucketList);

template <typename T>
const std::string_view HashTable<T>::HashTableStructName("kush::data::Struct");

template <typename T>
const std::string_view HashTable<T>::CreateFnName(
    "_ZN4kush4data6CreateEPNS0_9HashTableEl");

template <typename T>
const std::string_view HashTable<T>::InsertFnName(
    "_ZN4kush4data6InsertEPNS0_9HashTableEi");

template <typename T>
const std::string_view HashTable<T>::GetBucketFnName(
    "_ZN4kush4data9GetBucketEPNS0_9HashTableEi");

template <typename T>
const std::string_view HashTable<T>::GetAllBucketsFnName(
    "_ZN4kush4data13GetAllBucketsEPNS0_9HashTableEPNS0_10BucketListE");

template <typename T>
const std::string_view HashTable<T>::FreeFnName(
    "_ZN4kush4data4FreeEPNS0_9HashTableE");

template <typename T>
const std::string_view HashTable<T>::HashCombineFnName(
    "_ZN4kush4data11HashCombineEPil");

template <typename T>
HashTable<T>::HashTable(ProgramBuilder<T>& program, StructBuilder<T>& content)
    : program_(program),
      content_(content),
      content_type_(content_.Type()),
      value_(program_.Alloca(program.GetStructType(HashTableStructName))),
      hash_ptr_(program_.Alloca(program_.I32Type())),
      bucket_list_(program_.Alloca(
          program.GetStructType(BucketList<T>::BucketListStructName))) {
  auto& element_size = program_.SizeOf(content_type_);
  program_.Call(program.GetFunction(CreateFnName), {value_, element_size});
}

template <typename T>
HashTable<T>::~HashTable() {
  program_.Call(program_.GetFunction(FreeFnName), {value_});
}

template <typename T>
Struct<T> HashTable<T>::Insert(
    std::vector<std::reference_wrapper<proxy::Value<T>>> keys) {
  program_.Store(hash_ptr_, program_.ConstI32(0));
  for (auto& k : keys) {
    auto& k_hash = k.get().Hash();
    program_.Call(program_.GetFunction(HashCombineFnName), {hash_ptr_, k_hash});
  }
  auto& hash = program_.Load(hash_ptr_);

  auto& data =
      program_.Call(program_.GetFunction(InsertFnName), {value_, hash});
  auto& ptr = program_.PointerCast(data, program_.PointerType(content_type_));
  return Struct<T>(program_, content_, ptr);
}

template <typename T>
Vector<T> HashTable<T>::Get(
    std::vector<std::reference_wrapper<proxy::Value<T>>> keys) {
  program_.Store(hash_ptr_, program_.ConstI32(0));
  for (auto& k : keys) {
    auto& k_hash = k.get().Hash();
    program_.Call(program_.GetFunction(HashCombineFnName), {hash_ptr_, k_hash});
  }
  auto& hash = program_.Load(hash_ptr_);

  auto& bucket_ptr =
      program_.Call(program_.GetFunction(GetBucketFnName), {value_, hash});
  return Vector<T>(program_, content_, bucket_ptr);
}

template <typename T>
void HashTable<T>::ForwardDeclare(ProgramBuilder<T>& program) {
  auto& vector_ptr_type =
      program.PointerType(program.GetStructType(Vector<T>::VectorStructName));
  auto& bucket_list_struct_ptr = program.PointerType(
      program.GetStructType(BucketList<T>::BucketListStructName));

  auto& struct_type = program.StructType(
      {
          program.I64Type(),
          program.PointerType(program.I8Type()),
      },
      HashTableStructName);
  auto& struct_ptr = program.PointerType(struct_type);

  program.DeclareExternalFunction(CreateFnName, program.VoidType(),
                                  {struct_ptr, program.I64Type()});

  program.DeclareExternalFunction(InsertFnName,
                                  program.PointerType(program.I8Type()),
                                  {struct_ptr, program.I32Type()});

  program.DeclareExternalFunction(GetBucketFnName, vector_ptr_type,
                                  {struct_ptr, program.I32Type()});

  program.DeclareExternalFunction(FreeFnName, program.VoidType(), {struct_ptr});

  program.DeclareExternalFunction(GetAllBucketsFnName, program.VoidType(),
                                  {struct_ptr, bucket_list_struct_ptr});

  program.DeclareExternalFunction(
      HashCombineFnName, program.VoidType(),
      {program.PointerType(program.I32Type()), program.I64Type()});
}

template <typename T>
void HashTable<T>::ForEach(std::function<void(Struct<T>&)> handler) {
  program_.Call(program_.GetFunction(GetAllBucketsFnName),
                {value_, bucket_list_});

  BucketList<T> bucket_list(program_, content_, bucket_list_);

  proxy::Loop<T>(
      program_,
      [&](auto& loop) {
        auto i = proxy::Int32<T>(program_, 0);
        loop.AddLoopVariable(i);
      },
      [&](auto& loop) {
        auto i = loop.template GetLoopVariable<proxy::Int32<T>>(0);
        return i < bucket_list.Size();
      },
      [&](auto& loop) {
        auto i = loop.template GetLoopVariable<proxy::Int32<T>>(0);
        auto bucket = bucket_list[i];

        proxy::Loop<T>(
            program_,
            [&](auto& loop) {
              auto j = proxy::Int32<T>(program_, 0);
              loop.AddLoopVariable(j);
            },
            [&](auto& loop) {
              auto j = loop.template GetLoopVariable<proxy::Int32<T>>(0);
              return j < bucket.Size();
            },
            [&](auto& loop) {
              auto j = loop.template GetLoopVariable<proxy::Int32<T>>(0);
              auto data = bucket[j];

              handler(data);

              std::unique_ptr<proxy::Value<T>> next_j = (j + 1).ToPointer();
              return util::MakeVector(std::move(next_j));
            });

        std::unique_ptr<proxy::Value<T>> next_i = (i + 1).ToPointer();
        return util::MakeVector(std::move(next_i));
      });

  bucket_list.Reset();
}

INSTANTIATE_ON_IR(HashTable);

}  // namespace kush::compile::proxy