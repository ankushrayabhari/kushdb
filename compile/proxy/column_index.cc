#include "compile/proxy/column_index.h"

#include <iostream>
#include <memory>

#include "catalog/sql_type.h"
#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"
#include "runtime/column_index.h"
#include "runtime/column_index_bucket.h"
#include "runtime/memory_column_index.h"

namespace kush::compile::proxy {

constexpr std::string_view ColumnIndexBucketName(
    "kush::runtime::ColumnIndexBucket");

constexpr std::string_view FastForwardBucketName(
    "kush::runtime::ColumnIndexBucket::FastForwardBucket");

constexpr std::string_view ColumnIndexBucketGetName(
    "kush::runtime::ColumnIndexBucket::GetBucketValue");

constexpr std::string_view BucketListGetName(
    "kush::runtime::ColumnIndexBucket::BucketListGet");

constexpr std::string_view BucketListSortedIntersectionInitName(
    "kush::runtime::ColumnIndexBucket::BucketListSortedIntersectionInit");

constexpr std::string_view BucketListSortedIntersectionPopulateResultName(
    "kush::runtime::ColumnIndexBucket::"
    "BucketListSortedIntersectionPopulateResult");

constexpr std::string_view BucketListSortedIntersectionResultGetName(
    "kush::runtime::ColumnIndexBucket::BucketListSortedIntersectionResultGet");

ColumnIndexBucket::ColumnIndexBucket(khir::ProgramBuilder& program,
                                     khir::Value v)
    : program_(program), value_(v) {}

void ColumnIndexBucket::Copy(const ColumnIndexBucket& rhs) {
  auto st = program_.GetStructType(ColumnIndexBucketName);
  program_.StorePtr(
      program_.ConstGEP(st, value_, {0, 0}),
      program_.LoadPtr(program_.ConstGEP(st, rhs.value_, {0, 0})));
  program_.StoreI32(
      program_.ConstGEP(st, value_, {0, 1}),
      program_.LoadI32(program_.ConstGEP(st, rhs.value_, {0, 1})));
}

Int32 ColumnIndexBucket::FastForwardToStart(const Int32& last_tuple) {
  return Int32(program_,
               program_.Call(program_.GetFunction(FastForwardBucketName),
                             {value_, last_tuple.Get()}));
}

Int32 ColumnIndexBucket::Size() {
  auto size_ptr = program_.ConstGEP(
      program_.GetStructType(ColumnIndexBucketName), value_, {0, 1});
  return Int32(program_, program_.LoadI32(size_ptr));
}

Int32 ColumnIndexBucket::operator[](const Int32& v) {
  return Int32(program_,
               program_.Call(program_.GetFunction(ColumnIndexBucketGetName),
                             {value_, v.Get()}));
}

Bool ColumnIndexBucket::DoesNotExist() {
  auto st = program_.GetStructType(ColumnIndexBucketName);
  return Bool(program_, program_.IsNullPtr(program_.LoadPtr(
                            program_.ConstGEP(st, value_, {0, 0}))));
}

void ColumnIndexBucket::ForwardDeclare(khir::ProgramBuilder& program) {
  auto index_bucket_type = program.StructType(
      {program.PointerType(program.I32Type()), program.I32Type()},
      ColumnIndexBucketName);
  auto index_bucket_ptr_type = program.PointerType(index_bucket_type);

  program.DeclareExternalFunction(
      FastForwardBucketName, program.I32Type(),
      {index_bucket_ptr_type, program.I32Type()},
      reinterpret_cast<void*>(&runtime::FastForwardBucket));

  program.DeclareExternalFunction(
      ColumnIndexBucketGetName, program.I32Type(),
      {index_bucket_ptr_type, program.I32Type()},
      reinterpret_cast<void*>(&runtime::GetBucketValue));

  program.DeclareExternalFunction(
      BucketListGetName, index_bucket_ptr_type,
      {index_bucket_ptr_type, program.I32Type()},
      reinterpret_cast<void*>(&runtime::BucketListGet));

  program.DeclareExternalFunction(
      BucketListSortedIntersectionInitName, program.VoidType(),
      {index_bucket_ptr_type, program.I32Type(),
       program.PointerType(program.I32Type()), program.I32Type()},
      reinterpret_cast<void*>(&runtime::BucketListSortedIntersectionInit));

  program.DeclareExternalFunction(
      BucketListSortedIntersectionPopulateResultName, program.I32Type(),
      {index_bucket_ptr_type, program.I32Type(),
       program.PointerType(program.I32Type()),
       program.PointerType(program.I32Type()), program.I32Type()},
      reinterpret_cast<void*>(
          &runtime::BucketListSortedIntersectionPopulateResult));

  program.DeclareExternalFunction(
      BucketListSortedIntersectionResultGetName, program.I32Type(),
      {program.PointerType(program.I32Type()), program.I32Type()},
      reinterpret_cast<void*>(&runtime::BucketListSortedIntersectionResultGet));
}

khir::Value ColumnIndexBucket::Get() const { return value_; }

ColumnIndexBucketArray::ColumnIndexBucketArray(khir::ProgramBuilder& program,
                                               int max_size)
    : program_(program),
      value_(program.Alloca(program.GetStructType(ColumnIndexBucketName),
                            max_size)),
      sorted_intersection_idx_value_(
          program.Alloca(program_.I32Type(), max_size)),
      idx_value_(program_.Alloca(program_.I32Type())) {
  program_.StoreI32(idx_value_, program_.ConstI32(0));
}

Int32 ColumnIndexBucketArray::Size() {
  return Int32(program_, program_.LoadI32(idx_value_));
}

ColumnIndexBucket ColumnIndexBucketArray::Get(Int32 idx) {
  return ColumnIndexBucket(
      program_, program_.Call(program_.GetFunction(BucketListGetName),
                              {value_, idx.Get()}));
}

void ColumnIndexBucketArray::PushBack(const ColumnIndexBucket& bucket) {
  auto idx = Size();
  auto dest = Get(idx);
  dest.Copy(bucket);
  program_.StoreI32(idx_value_, (idx + 1).Get());
}

void ColumnIndexBucketArray::InitSortedIntersection(
    const proxy::Int32& next_tuple) {
  program_.Call(program_.GetFunction(BucketListSortedIntersectionInitName),
                {value_, program_.LoadI32(idx_value_),
                 sorted_intersection_idx_value_, next_tuple.Get()});
}

proxy::Int32 ColumnIndexBucketArray::PopulateSortedIntersectionResult(
    khir::Value result, int32_t result_max_size) {
  return proxy::Int32(
      program_,
      program_.Call(
          program_.GetFunction(BucketListSortedIntersectionPopulateResultName),
          {value_, program_.LoadI32(idx_value_), sorted_intersection_idx_value_,
           result, program_.ConstI32(result_max_size)}));
}

proxy::Int32 SortedIntersectionResultGet(khir::ProgramBuilder& program,
                                         khir::Value result,
                                         const proxy::Int32& idx) {
  return proxy::Int32(
      program, program.Call(program.GetFunction(
                                BucketListSortedIntersectionResultGetName),
                            {result, idx.Get()}));
}

template <catalog::SqlType S>
std::string_view TypeName() {
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
void MemoryColumnIndex<S>::Init() {}

template <catalog::SqlType S>
void MemoryColumnIndex<S>::Reset() {
  program_.Call(program_.GetFunction(FreeFnName<S>()),
                {program_.LoadPtr(value_)});
}

template <catalog::SqlType S>
MemoryColumnIndex<S>::MemoryColumnIndex(khir::ProgramBuilder& program,
                                        bool global)
    : program_(program),
      value_(global ? program.Global(false, false,
                                     program.PointerType(
                                         program.GetOpaqueType(TypeName<S>())),
                                     program.NullPtr(program.PointerType(
                                         program.GetOpaqueType(TypeName<S>()))))
                    : program_.Alloca(program.PointerType(
                          program.GetOpaqueType(TypeName<S>())))),
      get_value_(program.Global(
          false, true, program.GetStructType(ColumnIndexBucketName),
          program.ConstantStruct(
              program.GetStructType(ColumnIndexBucketName),
              {program.NullPtr(program.PointerType(program.I32Type())),
               program.ConstI32(0)}))) {
  program_.StorePtr(value_,
                    program_.Call(program_.GetFunction(CreateFnName<S>()), {}));
}

template <catalog::SqlType S>
MemoryColumnIndex<S>::MemoryColumnIndex(khir::ProgramBuilder& program,
                                        khir::Value v)
    : program_(program),
      value_(program_.Alloca(
          program.PointerType(program.GetOpaqueType(TypeName<S>())))),
      get_value_(program.Global(
          false, true, program.GetStructType(ColumnIndexBucketName),
          program.ConstantStruct(
              program.GetStructType(ColumnIndexBucketName),
              {program.NullPtr(program.PointerType(program.I32Type())),
               program.ConstI32(0)}))) {
  program_.StorePtr(value_, v);
}

template <catalog::SqlType S>
void MemoryColumnIndex<S>::Insert(const IRValue& v, const Int32& tuple_idx) {
  program_.Call(program_.GetFunction(InsertFnName<S>()),
                {program_.LoadPtr(value_), v.Get(), tuple_idx.Get()});
}

template <catalog::SqlType S>
ColumnIndexBucket MemoryColumnIndex<S>::GetBucket(const IRValue& v) {
  program_.Call(program_.GetFunction(GetBucketFnName<S>()),
                {program_.LoadPtr(value_), v.Get(), get_value_});
  return ColumnIndexBucket(program_, get_value_);
}

template <catalog::SqlType S>
khir::Value MemoryColumnIndex<S>::Serialize() {
  return program_.PointerCast(program_.LoadPtr(value_),
                              program_.PointerType(program_.I8Type()));
}

template <catalog::SqlType S>
std::unique_ptr<ColumnIndex> MemoryColumnIndex<S>::Regenerate(
    khir::ProgramBuilder& program, void* value) {
  return std::make_unique<MemoryColumnIndex<S>>(
      program, program.PointerCast(
                   program.ConstPtr(value),
                   program.PointerType(program.GetOpaqueType(TypeName<S>()))));
}

template <catalog::SqlType S>
void MemoryColumnIndex<S>::ForwardDeclare(khir::ProgramBuilder& program) {
  std::optional<khir::Type> index_type;
  if constexpr (catalog::SqlType::DATE == S) {
    index_type = program.GetOpaqueType(TypeName<S>());
  } else {
    index_type = program.OpaqueType(TypeName<S>());
  }
  auto index_ptr_type = program.PointerType(index_type.value());

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
  auto bucket_ptr_type =
      program.PointerType(program.GetStructType(ColumnIndexBucketName));
  program.DeclareExternalFunction(
      GetBucketFnName<S>(), program.VoidType(),
      {index_ptr_type, value_type.value(), bucket_ptr_type}, GetBucketFn<S>());
}

template class MemoryColumnIndex<catalog::SqlType::SMALLINT>;
template class MemoryColumnIndex<catalog::SqlType::INT>;
template class MemoryColumnIndex<catalog::SqlType::BIGINT>;
template class MemoryColumnIndex<catalog::SqlType::REAL>;
template class MemoryColumnIndex<catalog::SqlType::DATE>;
template class MemoryColumnIndex<catalog::SqlType::BOOLEAN>;
template class MemoryColumnIndex<catalog::SqlType::TEXT>;

constexpr std::string_view DiskColumnIndexName(
    "kush::runtime::ColumnIndex::ColumnIndex");

constexpr std::string_view DiskColumnIndexDataName(
    "kush::runtime::ColumnIndex::ColumnIndexData");

constexpr std::string_view DiskColumnIndexOpenFnName(
    "kush::runtime::ColumnIndex::Open");

constexpr std::string_view DiskColumnIndexCloseFnName(
    "kush::runtime::ColumnIndex::Close");

template <catalog::SqlType S>
std::string_view DiskColumnIndexGetBucketFnName() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return "kush::runtime::ColumnIndex::GetInt16";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "kush::runtime::ColumnIndex::GetInt32";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "kush::runtime::ColumnIndex::GetInt64";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "kush::runtime::ColumnIndex::GetFloat64";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "kush::runtime::ColumnIndex::GetInt8";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "kush::runtime::ColumnIndex::GetText";
  }
}

template <catalog::SqlType S>
void* DiskColumnIndexGetBucketFn() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return reinterpret_cast<void*>(runtime::ColumnIndex::GetInt16);
  } else if constexpr (catalog::SqlType::INT == S) {
    return reinterpret_cast<void*>(runtime::ColumnIndex::GetInt32);
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return reinterpret_cast<void*>(runtime::ColumnIndex::GetInt64);
  } else if constexpr (catalog::SqlType::REAL == S) {
    return reinterpret_cast<void*>(runtime::ColumnIndex::GetFloat64);
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return reinterpret_cast<void*>(runtime::ColumnIndex::GetInt8);
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return reinterpret_cast<void*>(runtime::ColumnIndex::GetText);
  }
}

template <catalog::SqlType S>
DiskColumnIndex<S>::DiskColumnIndex(khir::ProgramBuilder& program,
                                    std::string_view path)
    : program_(program),
      path_(path),
      path_value_(program.GlobalConstCharArray(path)),
      value_(program.Global(
          false, true, program.GetStructType(DiskColumnIndexName),
          program.ConstantStruct(
              program.GetStructType(DiskColumnIndexName),
              {program.NullPtr(program.PointerType(
                   program.GetOpaqueType(DiskColumnIndexDataName))),
               program.ConstI64(0)}))),
      get_value_(program.Global(
          false, true, program.GetStructType(ColumnIndexBucketName),
          program.ConstantStruct(
              program.GetStructType(ColumnIndexBucketName),
              {program.NullPtr(program.PointerType(program.I32Type())),
               program.ConstI32(0)}))) {}

template <catalog::SqlType S>
DiskColumnIndex<S>::DiskColumnIndex(khir::ProgramBuilder& program,
                                    std::string_view path, khir::Value v)
    : program_(program),
      path_(path),
      path_value_(program.GlobalConstCharArray(path)),
      value_(v),
      get_value_(program.Global(
          false, true, program.GetStructType(ColumnIndexBucketName),
          program.ConstantStruct(
              program.GetStructType(ColumnIndexBucketName),
              {program.NullPtr(program.PointerType(program.I32Type())),
               program.ConstI32(0)}))) {}

template <catalog::SqlType S>
void DiskColumnIndex<S>::Init() {
  program_.Call(program_.GetFunction(DiskColumnIndexOpenFnName),
                {value_, path_value_});
}

template <catalog::SqlType S>
void DiskColumnIndex<S>::Reset() {
  program_.Call(program_.GetFunction(DiskColumnIndexCloseFnName), {value_});
}

template <catalog::SqlType S>
ColumnIndexBucket DiskColumnIndex<S>::GetBucket(const IRValue& v) {
  program_.Call(program_.GetFunction(DiskColumnIndexGetBucketFnName<S>()),
                {value_, v.Get(), get_value_});
  return ColumnIndexBucket(program_, get_value_);
}

template <catalog::SqlType S>
khir::Value DiskColumnIndex<S>::Serialize() {
  return program_.PointerCast(value_, program_.PointerType(program_.I8Type()));
}

template <catalog::SqlType S>
std::unique_ptr<ColumnIndex> DiskColumnIndex<S>::Regenerate(
    khir::ProgramBuilder& program, void* value) {
  return std::make_unique<DiskColumnIndex<S>>(
      program, path_,
      program.PointerCast(
          program.ConstPtr(value),
          program.PointerType(program.GetStructType(DiskColumnIndexName))));
}

template <catalog::SqlType S>
void DiskColumnIndex<S>::ForwardDeclare(khir::ProgramBuilder& program) {
  if constexpr (S == catalog::SqlType::SMALLINT) {
    auto col_idx_data = program.OpaqueType(DiskColumnIndexDataName);
    auto st = program.StructType(
        {program.PointerType(col_idx_data), program.I64Type()},
        DiskColumnIndexName);
    auto struct_ptr = program.PointerType(st);
    auto string_type = program.PointerType(program.I8Type());
    program.DeclareExternalFunction(
        DiskColumnIndexOpenFnName, program.VoidType(),
        {struct_ptr, string_type},
        reinterpret_cast<void*>(runtime::ColumnIndex::Open));
    program.DeclareExternalFunction(
        DiskColumnIndexCloseFnName, program.VoidType(), {struct_ptr},
        reinterpret_cast<void*>(runtime::ColumnIndex::Close));
  }

  auto st = program.GetStructType(DiskColumnIndexName);
  auto struct_ptr = program.PointerType(st);

  std::optional<khir::Type> key_type;
  if constexpr (catalog::SqlType::SMALLINT == S) {
    key_type = program.I16Type();
  } else if constexpr (catalog::SqlType::INT == S) {
    key_type = program.I32Type();
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    key_type = program.I64Type();
  } else if constexpr (catalog::SqlType::REAL == S) {
    key_type = program.F64Type();
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    key_type = program.I1Type();
  } else if constexpr (catalog::SqlType::TEXT == S) {
    key_type =
        program.PointerType(program.GetStructType(String::StringStructName));
  }

  auto bucket_type = program.GetStructType(ColumnIndexBucketName);
  auto bucket_ptr_type = program.PointerType(bucket_type);

  program.DeclareExternalFunction(
      DiskColumnIndexGetBucketFnName<S>(), program.VoidType(),
      {struct_ptr, key_type.value(), bucket_ptr_type},
      DiskColumnIndexGetBucketFn<S>());
}

template class DiskColumnIndex<catalog::SqlType::SMALLINT>;
template class DiskColumnIndex<catalog::SqlType::INT>;
template class DiskColumnIndex<catalog::SqlType::BIGINT>;
template class DiskColumnIndex<catalog::SqlType::REAL>;
template class DiskColumnIndex<catalog::SqlType::DATE>;
template class DiskColumnIndex<catalog::SqlType::BOOLEAN>;
template class DiskColumnIndex<catalog::SqlType::TEXT>;

}  // namespace kush::compile::proxy