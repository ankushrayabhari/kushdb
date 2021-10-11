#include "compile/proxy/column_index.h"

#include <iostream>
#include <memory>

#include "catalog/sql_type.h"
#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"
#include "runtime/memory_column_index.h"

namespace kush::compile::proxy {

std::string_view bucket_type_name("kush::runtime::MemoryColumnIndex::Bucket");

template <catalog::SqlType S>
std::string_view ColumnIndexImpl<S>::TypeName() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return "kush::runtime::MemoryColumnIndex::Int16Index";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "kush::runtime::MemoryColumnIndex::Int32Index";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "kush::runtime::MemoryColumnIndex::Int64Index";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "kush::runtime::MemoryColumnIndex::Float64Index";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "kush::runtime::MemoryColumnIndex::Int8Index";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "kush::runtime::MemoryColumnIndex::TextIndex";
  }
}

template <catalog::SqlType S>
std::string_view CreateFnName() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return "kush::runtime::MemoryColumnIndex::CreateInt16Index";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "kush::runtime::MemoryColumnIndex::CreateInt32Index";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "kush::runtime::MemoryColumnIndex::CreateInt64Index";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "kush::runtime::MemoryColumnIndex::CreateFloat64Index";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "kush::runtime::MemoryColumnIndex::CreateInt8Index";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "kush::runtime::MemoryColumnIndex::CreateTextIndex";
  }
}

template <catalog::SqlType S>
void* CreateFn() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return reinterpret_cast<void*>(
        &runtime::MemoryColumnIndex::CreateInt16Index);
  } else if constexpr (catalog::SqlType::INT == S) {
    return reinterpret_cast<void*>(
        &runtime::MemoryColumnIndex::CreateInt32Index);
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return reinterpret_cast<void*>(
        &runtime::MemoryColumnIndex::CreateInt64Index);
  } else if constexpr (catalog::SqlType::REAL == S) {
    return reinterpret_cast<void*>(
        &runtime::MemoryColumnIndex::CreateFloat64Index);
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return reinterpret_cast<void*>(
        &runtime::MemoryColumnIndex::CreateInt8Index);
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return reinterpret_cast<void*>(
        &runtime::MemoryColumnIndex::CreateTextIndex);
  }
}

template <catalog::SqlType S>
std::string_view FreeFnName() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return "kush::runtime::MemoryColumnIndex::FreeInt16Index";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "kush::runtime::MemoryColumnIndex::FreeInt32Index";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "kush::runtime::MemoryColumnIndex::FreeInt64Index";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "kush::runtime::MemoryColumnIndex::FreeFloat64Index";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "kush::runtime::MemoryColumnIndex::FreeInt8Index";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "kush::runtime::MemoryColumnIndex::FreeTextIndex";
  }
}

template <catalog::SqlType S>
void* FreeFn() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return reinterpret_cast<void*>(&runtime::MemoryColumnIndex::FreeInt16Index);
  } else if constexpr (catalog::SqlType::INT == S) {
    return reinterpret_cast<void*>(&runtime::MemoryColumnIndex::FreeInt32Index);
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return reinterpret_cast<void*>(&runtime::MemoryColumnIndex::FreeInt64Index);
  } else if constexpr (catalog::SqlType::REAL == S) {
    return reinterpret_cast<void*>(
        &runtime::MemoryColumnIndex::FreeFloat64Index);
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return reinterpret_cast<void*>(&runtime::MemoryColumnIndex::FreeInt8Index);
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return reinterpret_cast<void*>(&runtime::MemoryColumnIndex::FreeTextIndex);
  }
}

template <catalog::SqlType S>
std::string_view GetBucketFnName() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return "kush::runtime::MemoryColumnIndex::GetBucketInt16Index";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "kush::runtime::MemoryColumnIndex::GetBucketInt32Index";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "kush::runtime::MemoryColumnIndex::GetBucketInt64Index";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "kush::runtime::MemoryColumnIndex::GetBucketFloat64Index";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "kush::runtime::MemoryColumnIndex::GetBucketInt8Index";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "kush::runtime::MemoryColumnIndex::GetBucketTextIndex";
  }
}

template <catalog::SqlType S>
void* GetBucketFn() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return reinterpret_cast<void*>(
        &runtime::MemoryColumnIndex::GetBucketInt16Index);
  } else if constexpr (catalog::SqlType::INT == S) {
    return reinterpret_cast<void*>(
        &runtime::MemoryColumnIndex::GetBucketInt32Index);
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return reinterpret_cast<void*>(
        &runtime::MemoryColumnIndex::GetBucketInt64Index);
  } else if constexpr (catalog::SqlType::REAL == S) {
    return reinterpret_cast<void*>(
        &runtime::MemoryColumnIndex::GetBucketFloat64Index);
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return reinterpret_cast<void*>(
        &runtime::MemoryColumnIndex::GetBucketInt8Index);
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return reinterpret_cast<void*>(
        &runtime::MemoryColumnIndex::GetBucketTextIndex);
  }
}

constexpr std::string_view FastForwardBucketName(
    "kush::runtime::MemoryColumnIndex::FastForwardBucket");

constexpr std::string_view BucketSizeName(
    "kush::runtime::MemoryColumnIndex::BucketSize");

constexpr std::string_view BucketGetName(
    "kush::runtime::MemoryColumnIndex::BucketGet");

constexpr std::string_view CreateBucketListName(
    "kush::runtime::MemoryColumnIndex::CreateBucketList");

constexpr std::string_view BucketListGetName(
    "kush::runtime::MemoryColumnIndex::BucketListGet");

constexpr std::string_view BucketListPushBackName(
    "kush::runtime::MemoryColumnIndex::BucketListPushBack");

constexpr std::string_view BucketListSizeName(
    "kush::runtime::MemoryColumnIndex::BucketListSize");

constexpr std::string_view FreeBucketListName(
    "kush::runtime::MemoryColumnIndex::FreeBucketList");

template <catalog::SqlType S>
std::string_view InsertFnName() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return "kush::runtime::MemoryColumnIndex::InsertInt16Index";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "kush::runtime::MemoryColumnIndex::InsertInt32Index";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "kush::runtime::MemoryColumnIndex::InsertInt64Index";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "kush::runtime::MemoryColumnIndex::InsertFloat64Index";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "kush::runtime::MemoryColumnIndex::InsertInt8Index";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "kush::runtime::MemoryColumnIndex::InsertTextIndex";
  }
}

template <catalog::SqlType S>
void* InsertFn() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return reinterpret_cast<void*>(
        &runtime::MemoryColumnIndex::InsertInt16Index);
  } else if constexpr (catalog::SqlType::INT == S) {
    return reinterpret_cast<void*>(
        &runtime::MemoryColumnIndex::InsertInt32Index);
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return reinterpret_cast<void*>(
        &runtime::MemoryColumnIndex::InsertInt64Index);
  } else if constexpr (catalog::SqlType::REAL == S) {
    return reinterpret_cast<void*>(
        &runtime::MemoryColumnIndex::InsertFloat64Index);
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return reinterpret_cast<void*>(
        &runtime::MemoryColumnIndex::InsertInt8Index);
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return reinterpret_cast<void*>(
        &runtime::MemoryColumnIndex::InsertTextIndex);
  }
}

template <catalog::SqlType S>
void ColumnIndexImpl<S>::Reset() {
  program_.Call(program_.GetFunction(FreeFnName<S>()),
                {program_.LoadPtr(value_)});
}

template <catalog::SqlType S>
ColumnIndexImpl<S>::ColumnIndexImpl(khir::ProgramBuilder& program, bool global)
    : program_(program),
      value_(global ? program.Global(false, false,
                                     program.PointerType(
                                         program.GetOpaqueType(TypeName())),
                                     program.NullPtr(program.PointerType(
                                         program.GetOpaqueType(TypeName()))))
                    : program_.Alloca(program.PointerType(
                          program.GetOpaqueType(TypeName())))) {
  program_.StorePtr(value_,
                    program_.Call(program_.GetFunction(CreateFnName<S>()), {}));
}

template <catalog::SqlType S>
ColumnIndexImpl<S>::ColumnIndexImpl(khir::ProgramBuilder& program,
                                    khir::Value v)
    : program_(program),
      value_(program_.Alloca(
          program.PointerType(program.GetOpaqueType(TypeName())))) {
  program_.StorePtr(value_, v);
}

template <catalog::SqlType S>
khir::Value ColumnIndexImpl<S>::Get() const {
  return program_.LoadPtr(value_);
}

template <catalog::SqlType S>
catalog::SqlType ColumnIndexImpl<S>::Type() const {
  return S;
}

template <catalog::SqlType S>
void ColumnIndexImpl<S>::Insert(const IRValue& v, const Int32& tuple_idx) {
  program_.Call(program_.GetFunction(InsertFnName<S>()),
                {program_.LoadPtr(value_), v.Get(), tuple_idx.Get()});
}

template <catalog::SqlType S>
IndexBucket ColumnIndexImpl<S>::GetBucket(const IRValue& v) {
  return IndexBucket(program_,
                     program_.Call(program_.GetFunction(GetBucketFnName<S>()),
                                   {program_.LoadPtr(value_), v.Get()}));
}

template <catalog::SqlType S>
khir::Value ColumnIndexImpl<S>::Serialize() {
  return program_.PointerCast(Get(), program_.PointerType(program_.I8Type()));
}

template <catalog::SqlType S>
std::unique_ptr<ColumnIndex> ColumnIndexImpl<S>::Regenerate(
    khir::ProgramBuilder& program, void* value) {
  return std::make_unique<ColumnIndexImpl<S>>(
      program, program.PointerCast(
                   program.ConstPtr(value),
                   program.PointerType(program.GetOpaqueType(TypeName()))));
}

template <catalog::SqlType S>
void ColumnIndexImpl<S>::ForwardDeclare(khir::ProgramBuilder& program) {
  std::optional<khir::Type> index_type;
  if constexpr (catalog::SqlType::DATE == S) {
    index_type = program.GetOpaqueType(TypeName());
  } else {
    index_type = program.OpaqueType(TypeName());
  }
  auto index_ptr_type = program.PointerType(index_type.value());

  std::optional<khir::Type> bucket_type;
  if constexpr (catalog::SqlType::SMALLINT == S) {
    bucket_type = program.OpaqueType(bucket_type_name);
  } else {
    bucket_type = program.GetOpaqueType(bucket_type_name);
  }
  auto bucket_ptr_type = program.PointerType(bucket_type.value());

  std::optional<khir::Type> value_type;
  if constexpr (catalog::SqlType::SMALLINT == S) {
    value_type = program.I16Type();
  } else if constexpr (catalog::SqlType::INT == S) {
    value_type = program.I32Type();
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    value_type = program.I64Type();
  } else if constexpr (catalog::SqlType::REAL == S) {
    value_type = program.F64Type();
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    value_type = program.I1Type();
  } else if constexpr (catalog::SqlType::TEXT == S) {
    value_type =
        program.PointerType(program.GetStructType(String::StringStructName));
  }

  program.DeclareExternalFunction(CreateFnName<S>(), index_ptr_type, {},
                                  CreateFn<S>());
  program.DeclareExternalFunction(FreeFnName<S>(), program.VoidType(),
                                  {index_ptr_type}, FreeFn<S>());
  program.DeclareExternalFunction(
      InsertFnName<S>(), program.VoidType(),
      {index_ptr_type, value_type.value(), program.I32Type()}, InsertFn<S>());

  // Bucket related functionality
  program.DeclareExternalFunction(GetBucketFnName<S>(), bucket_ptr_type,
                                  {index_ptr_type, value_type.value()},
                                  GetBucketFn<S>());
  program.DeclareExternalFunction(
      FastForwardBucketName, program.I32Type(),
      {bucket_ptr_type, program.I32Type()},
      reinterpret_cast<void*>(&runtime::MemoryColumnIndex::FastForwardBucket));
  program.DeclareExternalFunction(
      BucketSizeName, program.I32Type(), {bucket_ptr_type},
      reinterpret_cast<void*>(&runtime::MemoryColumnIndex::BucketSize));
  program.DeclareExternalFunction(
      BucketGetName, program.I32Type(), {bucket_ptr_type, program.I32Type()},
      reinterpret_cast<void*>(&runtime::MemoryColumnIndex::BucketGet));

  // Bucket list related functionality
  auto bucket_list_type = program.I8Type();
  auto bucket_list_ptr_type = program.PointerType(bucket_list_type);
  program.DeclareExternalFunction(
      CreateBucketListName, bucket_list_ptr_type, {},
      reinterpret_cast<void*>(&runtime::MemoryColumnIndex::CreateBucketList));
  program.DeclareExternalFunction(
      FreeBucketListName, program.VoidType(), {bucket_list_ptr_type},
      reinterpret_cast<void*>(&runtime::MemoryColumnIndex::FreeBucketList));
  program.DeclareExternalFunction(
      BucketListSizeName, program.I32Type(), {bucket_list_ptr_type},
      reinterpret_cast<void*>(&runtime::MemoryColumnIndex::BucketListSize));
  program.DeclareExternalFunction(
      BucketListGetName, bucket_ptr_type,
      {bucket_list_ptr_type, program.I32Type()},
      reinterpret_cast<void*>(&runtime::MemoryColumnIndex::BucketListGet));
  program.DeclareExternalFunction(
      BucketListPushBackName, program.VoidType(),
      {bucket_list_ptr_type, bucket_ptr_type},
      reinterpret_cast<void*>(&runtime::MemoryColumnIndex::BucketListPushBack));
}

IndexBucket::IndexBucket(khir::ProgramBuilder& program, khir::Value v)
    : program_(program), value_(v) {}

Int32 IndexBucket::FastForwardToStart(const Int32& last_tuple) {
  return Int32(program_,
               program_.Call(program_.GetFunction(FastForwardBucketName),
                             {value_, last_tuple.Get()}));
}

Int32 IndexBucket::Size() {
  return Int32(program_,
               program_.Call(program_.GetFunction(BucketSizeName), {value_}));
}

Int32 IndexBucket::operator[](const Int32& v) {
  return Int32(program_, program_.Call(program_.GetFunction(BucketGetName),
                                       {value_, v.Get()}));
}

Bool IndexBucket::DoesNotExist() {
  return Bool(program_, program_.IsNullPtr(value_));
}

khir::Value IndexBucket::Get() const { return value_; }

IndexBucketList::IndexBucketList(khir::ProgramBuilder& program)
    : program_(program),
      value_(program.Alloca(program.PointerType(program.I8Type()))) {
  program_.StorePtr(
      value_, program_.Call(program_.GetFunction(CreateBucketListName), {}));
}

void IndexBucketList::Reset() {
  program_.Call(program_.GetFunction(FreeBucketListName),
                {program_.LoadPtr(value_)});
}

Int32 IndexBucketList::Size() {
  return Int32(program_, program_.Call(program_.GetFunction(BucketListSizeName),
                                       {program_.LoadPtr(value_)}));
}

IndexBucket IndexBucketList::operator[](const Int32& idx) {
  return IndexBucket(program_,
                     program_.Call(program_.GetFunction(BucketListGetName),
                                   {
                                       program_.LoadPtr(value_),
                                       idx.Get(),
                                   }));
}

void IndexBucketList::PushBack(const IndexBucket& bucket) {
  program_.Call(program_.GetFunction(BucketListPushBackName),
                {program_.LoadPtr(value_), bucket.Get()});
}

template class ColumnIndexImpl<catalog::SqlType::SMALLINT>;
template class ColumnIndexImpl<catalog::SqlType::INT>;
template class ColumnIndexImpl<catalog::SqlType::BIGINT>;
template class ColumnIndexImpl<catalog::SqlType::REAL>;
template class ColumnIndexImpl<catalog::SqlType::DATE>;
template class ColumnIndexImpl<catalog::SqlType::BOOLEAN>;
template class ColumnIndexImpl<catalog::SqlType::TEXT>;

}  // namespace kush::compile::proxy