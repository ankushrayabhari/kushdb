#include "compile/proxy/hash_table.h"

#include <functional>
#include <vector>

#include "compile/proxy/control_flow/if.h"
#include "compile/proxy/control_flow/loop.h"
#include "compile/proxy/struct.h"
#include "compile/proxy/vector.h"
#include "execution/query_state.h"
#include "khir/program_builder.h"
#include "runtime/hash_table.h"
#include "util/vector_util.h"

namespace kush::compile::proxy {

namespace {
constexpr std::string_view BucketListStructName(
    "kush::runtime::HashTable::BucketList");
constexpr std::string_view HashTableStructName(
    "kush::runtime::HashTable::HashTable");
constexpr std::string_view CreateFnName("kush::runtime::HashTable::Create");
constexpr std::string_view InsertFnName("kush::runtime::HashTable::Insert");
constexpr std::string_view GetBucketFnName(
    "kush::runtime::HashTable::GetBucket");
constexpr std::string_view GetAllBucketsFnName(
    "kush::runtime::HashTable::GetAllBuckets");
constexpr std::string_view FreeFnName("kush::runtime::HashTable::Free");
constexpr std::string_view HashCombineFnName(
    "kush::runtime::HashTable::HashCombine");
constexpr std::string_view BucketListSizeFnName =
    "kush::runtime::HashTable::BucketListSize";
constexpr std::string_view BucketListFreeFnName =
    "kush::runtime::HashTable::BucketListFree";
constexpr std::string_view BucketListGetBucketIdxFnName =
    "kush::runtime::HashTable::GetBucketIdx";
}  // namespace

class BucketList {
 public:
  BucketList(khir::ProgramBuilder& program, StructBuilder& content,
             khir::Value& value)
      : program_(program), content_(content), value_(value) {}

  Vector operator[](const Int32& i) {
    auto ptr = program_.Call(program_.GetFunction(BucketListGetBucketIdxFnName),
                             {value_, i.Get()});
    return Vector(program_, content_, ptr);
  }

  Int32 Size() {
    return Int32(
        program_,
        program_.Call(program_.GetFunction(BucketListSizeFnName), {value_}));
  }

  void Reset() {
    program_.Call(program_.GetFunction(BucketListFreeFnName), {value_});
  }

  static void ForwardDeclare(khir::ProgramBuilder& program) {
    auto vector_ptr_type =
        program.PointerType(program.GetStructType(Vector::VectorStructName));

    auto bucket_list_struct = program.StructType(
        {program.I32Type(), program.PointerType(vector_ptr_type)},
        BucketListStructName);
    auto bucket_list_struct_ptr = program.PointerType(bucket_list_struct);

    program.DeclareExternalFunction(
        BucketListGetBucketIdxFnName, vector_ptr_type,
        {bucket_list_struct_ptr, program.I32Type()},
        reinterpret_cast<void*>(&runtime::HashTable::GetBucketIdx));
    program.DeclareExternalFunction(
        BucketListSizeFnName, program.I32Type(), {bucket_list_struct_ptr},
        reinterpret_cast<void*>(&runtime::HashTable::BucketListSize));
    program.DeclareExternalFunction(
        BucketListFreeFnName, program.VoidType(), {bucket_list_struct_ptr},
        reinterpret_cast<void*>(&runtime::HashTable::BucketListFree));
  }

 private:
  khir::ProgramBuilder& program_;
  StructBuilder& content_;
  khir::Value& value_;
};

HashTable::HashTable(khir::ProgramBuilder& program,
                     execution::QueryState& state, StructBuilder& content)
    : program_(program),
      content_(content),
      content_type_(content_.Type()),
      value_(program_.PointerCast(
          program.ConstPtr(
              state.Allocate<kush::runtime::HashTable::HashTable>()),
          program.PointerType(program.GetStructType(HashTableStructName)))),
      hash_ptr_(program_.Global(program_.I32Type(), program.ConstI32(0))),
      bucket_list_(program_.Global(
          program.GetStructType(BucketListStructName),
          program.ConstantStruct(
              program.GetStructType(BucketListStructName),
              {
                  program.ConstI32(0),
                  program.NullPtr(program.PointerType(
                      program.GetStructType(Vector::VectorStructName))),
              }))) {}

void HashTable::Init() {
  auto element_size = program_.SizeOf(content_type_);
  program_.Call(program_.GetFunction(CreateFnName), {value_, element_size});
}

void HashTable::Reset() {
  program_.Call(program_.GetFunction(FreeFnName), {value_});
}

Struct HashTable::Insert(const std::vector<SQLValue>& keys) {
  program_.StoreI32(hash_ptr_, program_.ConstI32(0));
  for (auto& k : keys) {
    If(
        program_, k.IsNull(),
        [&]() {
          program_.Call(program_.GetFunction(HashCombineFnName),
                        {hash_ptr_, program_.ConstI64(0)});
        },
        [&]() {
          program_.Call(program_.GetFunction(HashCombineFnName),
                        {hash_ptr_, k.Get().Hash().Get()});
        });
  }
  auto hash = program_.LoadI32(hash_ptr_);

  auto data = program_.Call(program_.GetFunction(InsertFnName), {value_, hash});
  auto ptr = program_.PointerCast(data, program_.PointerType(content_type_));
  return Struct(program_, content_, ptr);
}

Vector HashTable::Get(const std::vector<SQLValue>& keys) {
  program_.StoreI32(hash_ptr_, program_.ConstI32(0));
  for (auto& k : keys) {
    If(
        program_, k.IsNull(),
        [&]() {
          program_.Call(program_.GetFunction(HashCombineFnName),
                        {hash_ptr_, program_.ConstI64(0)});
        },
        [&]() {
          program_.Call(program_.GetFunction(HashCombineFnName),
                        {hash_ptr_, k.Get().Hash().Get()});
        });
  }
  auto hash = program_.LoadI32(hash_ptr_);

  auto bucket_ptr =
      program_.Call(program_.GetFunction(GetBucketFnName), {value_, hash});
  return Vector(program_, content_, bucket_ptr);
}

void HashTable::ForwardDeclare(khir::ProgramBuilder& program) {
  BucketList::ForwardDeclare(program);

  auto vector_ptr_type =
      program.PointerType(program.GetStructType(Vector::VectorStructName));
  auto bucket_list_struct_ptr =
      program.PointerType(program.GetStructType(BucketListStructName));

  auto struct_type = program.StructType(
      {
          program.I64Type(),
          program.PointerType(program.I8Type()),
      },
      HashTableStructName);
  auto struct_ptr = program.PointerType(struct_type);

  program.DeclareExternalFunction(
      CreateFnName, program.VoidType(), {struct_ptr, program.I64Type()},
      reinterpret_cast<void*>(&runtime::HashTable::Create));

  program.DeclareExternalFunction(
      InsertFnName, program.PointerType(program.I8Type()),
      {struct_ptr, program.I32Type()},
      reinterpret_cast<void*>(&runtime::HashTable::Insert));

  program.DeclareExternalFunction(
      GetBucketFnName, vector_ptr_type, {struct_ptr, program.I32Type()},
      reinterpret_cast<void*>(&runtime::HashTable::GetBucket));

  program.DeclareExternalFunction(
      FreeFnName, program.VoidType(), {struct_ptr},
      reinterpret_cast<void*>(&runtime::HashTable::Free));

  program.DeclareExternalFunction(
      GetAllBucketsFnName, program.VoidType(),
      {struct_ptr, bucket_list_struct_ptr},
      reinterpret_cast<void*>(&runtime::HashTable::GetAllBuckets));

  program.DeclareExternalFunction(
      HashCombineFnName, program.VoidType(),
      {program.PointerType(program.I32Type()), program.I64Type()},
      reinterpret_cast<void*>(&runtime::HashTable::HashCombine));
}

void HashTable::ForEach(std::function<void(Struct&)> handler) {
  program_.Call(program_.GetFunction(GetAllBucketsFnName),
                {value_, bucket_list_});

  BucketList bucket_list(program_, content_, bucket_list_);

  Loop(
      program_, [&](auto& loop) { loop.AddLoopVariable(Int32(program_, 0)); },
      [&](auto& loop) {
        auto i = loop.template GetLoopVariable<Int32>(0);
        return i < bucket_list.Size();
      },
      [&](auto& loop) {
        auto i = loop.template GetLoopVariable<Int32>(0);
        auto bucket = bucket_list[i];

        Loop(
            program_,
            [&](auto& loop) { loop.AddLoopVariable(Int32(program_, 0)); },
            [&](auto& loop) {
              auto j = loop.template GetLoopVariable<Int32>(0);
              return j < bucket.Size();
            },
            [&](auto& loop) {
              auto j = loop.template GetLoopVariable<Int32>(0);
              auto data = bucket[j];

              handler(data);

              return loop.Continue(j + 1);
            });

        return loop.Continue(i + 1);
      });

  bucket_list.Reset();
}

}  // namespace kush::compile::proxy