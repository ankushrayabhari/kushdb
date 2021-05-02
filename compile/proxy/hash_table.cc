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

constexpr std::string_view BucketListStructName("kush::data::BucketList");

template <typename T>
class BucketList {
 public:
  BucketList(ProgramBuilder<T>& program, StructBuilder<T>& content,
             typename ProgramBuilder<T>::Value& value)
      : program_(program), content_(content), value_(value) {}

  Vector<T> operator[](const Int32<T>& i) {
    auto ptr = program_.Call(program_.GetFunction(BucketListGetBucketIdxFnName),
                             {value_, i.Get()});
    return Vector<T>(program_, content_, ptr);
  }

  Int32<T> Size() {
    return proxy::Int32<T>(
        program_,
        program_.Call(program_.GetFunction(BucketListSizeFnName), {value_}));
  }

  void Reset() {
    program_.Call(program_.GetFunction(BucketListFreeFnName), {value_});
  }

  static void ForwardDeclare(ProgramBuilder<T>& program) {
    auto vector_ptr_type =
        program.PointerType(program.GetStructType(Vector<T>::VectorStructName));

    auto bucket_list_struct = program.StructType(
        {program.I32Type(), program.PointerType(vector_ptr_type)},
        BucketListStructName);
    auto bucket_list_struct_ptr = program.PointerType(bucket_list_struct);

    program.DeclareExternalFunction(
        BucketListGetBucketIdxFnName, vector_ptr_type,
        {bucket_list_struct_ptr, program.I32Type()});
    program.DeclareExternalFunction(BucketListSizeFnName, program.I32Type(),
                                    {bucket_list_struct_ptr});
    program.DeclareExternalFunction(BucketListFreeFnName, program.VoidType(),
                                    {bucket_list_struct_ptr});
  }

 private:
  static constexpr std::string_view BucketListSizeFnName =
      "_ZN4kush4data4SizeEPNS0_10BucketListE";

  static constexpr std::string_view BucketListFreeFnName =
      "_ZN4kush4data4FreeEPNS0_10BucketListE";

  static constexpr std::string_view BucketListGetBucketIdxFnName =
      "_ZN4kush4data12GetBucketIdxEPNS0_10BucketListEi";

  ProgramBuilder<T>& program_;
  StructBuilder<T>& content_;
  typename ProgramBuilder<T>::Value& value_;
};

INSTANTIATE_ON_IR(BucketList);

constexpr std::string_view HashTableStructName("kush::data::Struct");
constexpr std::string_view CreateFnName(
    "_ZN4kush4data6CreateEPNS0_9HashTableEl");
constexpr std::string_view InsertFnName(
    "_ZN4kush4data6InsertEPNS0_9HashTableEi");
constexpr std::string_view GetBucketFnName(
    "_ZN4kush4data9GetBucketEPNS0_9HashTableEi");
constexpr std::string_view GetAllBucketsFnName(
    "_ZN4kush4data13GetAllBucketsEPNS0_9HashTableEPNS0_10BucketListE");
constexpr std::string_view FreeFnName("_ZN4kush4data4FreeEPNS0_9HashTableE");
constexpr std::string_view HashCombineFnName("_ZN4kush4data11HashCombineEPil");

template <typename T>
HashTable<T>::HashTable(ProgramBuilder<T>& program, StructBuilder<T>& content)
    : program_(program),
      content_(content),
      content_type_(content_.Type()),
      value_(program_.Alloca(program.GetStructType(HashTableStructName))),
      hash_ptr_(program_.Alloca(program_.I32Type())),
      bucket_list_(
          program_.Alloca(program.GetStructType(BucketListStructName))) {
  auto element_size = program_.SizeOf(content_type_);
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
    auto k_hash = k.get().Hash();
    program_.Call(program_.GetFunction(HashCombineFnName), {hash_ptr_, k_hash});
  }
  auto hash = program_.Load(hash_ptr_);

  auto data = program_.Call(program_.GetFunction(InsertFnName), {value_, hash});
  auto ptr = program_.PointerCast(data, program_.PointerType(content_type_));
  return Struct<T>(program_, content_, ptr);
}

template <typename T>
Vector<T> HashTable<T>::Get(
    std::vector<std::reference_wrapper<proxy::Value<T>>> keys) {
  program_.Store(hash_ptr_, program_.ConstI32(0));
  for (auto& k : keys) {
    auto k_hash = k.get().Hash();
    program_.Call(program_.GetFunction(HashCombineFnName), {hash_ptr_, k_hash});
  }
  auto hash = program_.Load(hash_ptr_);

  auto bucket_ptr =
      program_.Call(program_.GetFunction(GetBucketFnName), {value_, hash});
  return Vector<T>(program_, content_, bucket_ptr);
}

template <typename T>
void HashTable<T>::ForwardDeclare(ProgramBuilder<T>& program) {
  BucketList<T>::ForwardDeclare(program);

  auto vector_ptr_type =
      program.PointerType(program.GetStructType(Vector<T>::VectorStructName));
  auto bucket_list_struct_ptr =
      program.PointerType(program.GetStructType(BucketListStructName));

  auto struct_type = program.StructType(
      {
          program.I64Type(),
          program.PointerType(program.I8Type()),
      },
      HashTableStructName);
  auto struct_ptr = program.PointerType(struct_type);

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
      [&](auto& loop) { loop.AddLoopVariable(proxy::Int32<T>(program_, 0)); },
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
              loop.AddLoopVariable(proxy::Int32<T>(program_, 0));
            },
            [&](auto& loop) {
              auto j = loop.template GetLoopVariable<proxy::Int32<T>>(0);
              return j < bucket.Size();
            },
            [&](auto& loop) {
              auto j = loop.template GetLoopVariable<proxy::Int32<T>>(0);
              auto data = bucket[j];

              handler(data);

              return loop.Continue(j + 1);
            });

        return loop.Continue(i + 1);
      });

  bucket_list.Reset();
}

INSTANTIATE_ON_IR(HashTable);

}  // namespace kush::compile::proxy