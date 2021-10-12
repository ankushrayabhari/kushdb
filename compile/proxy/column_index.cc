#include "compile/proxy/column_index.h"

#include <iostream>
#include <memory>

#include "catalog/sql_type.h"
#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"
#include "runtime/memory_column_index.h"

namespace kush::compile::proxy {

constexpr std::string_view ColumnIndexBucketName(
    "kush::runtime::ColumnIndexBucket");

constexpr std::string_view FastForwardBucketName(
    "kush::runtime::ColumnIndexBucket::FastForwardBucket");

constexpr std::string_view ColumnIndexBucketGetName(
    "kush::runtime::ColumnIndexBucket::GetBucketValue");

ColumnIndexBucket::ColumnIndexBucket(khir::ProgramBuilder& program)
    : program_(program),
      value_(program_.Alloca(program_.GetStructType(ColumnIndexBucketName))) {}

ColumnIndexBucket::ColumnIndexBucket(khir::ProgramBuilder& program,
                                     khir::Value v)
    : program_(program), value_(v) {}

void ColumnIndexBucket::Copy(const ColumnIndexBucket& rhs) {
  auto st = program_.GetStructType(ColumnIndexBucketName);
  program_.StorePtr(
      program_.GetElementPtr(st, value_, {0, 0}),
      program_.LoadPtr(program_.GetElementPtr(st, rhs.value_, {0, 0})));
  program_.StoreI32(
      program_.GetElementPtr(st, value_, {0, 1}),
      program_.LoadI32(program_.GetElementPtr(st, rhs.value_, {0, 1})));
}

Int32 ColumnIndexBucket::FastForwardToStart(const Int32& last_tuple) {
  return Int32(program_,
               program_.Call(program_.GetFunction(FastForwardBucketName),
                             {value_, last_tuple.Get()}));
}

Int32 ColumnIndexBucket::Size() {
  auto size_ptr = program_.GetElementPtr(
      program_.GetStructType(ColumnIndexBucketName), value_, {0, 1});
  return Int32(program_, program_.LoadI32(size_ptr));
}

Int32 ColumnIndexBucket::operator[](const Int32& v) {
  return Int32(program_,
               program_.Call(program_.GetFunction(ColumnIndexBucketGetName),
                             {value_, v.Get()}));
}

Bool ColumnIndexBucket::DoesNotExist() {
  return Bool(program_, program_.IsNullPtr(value_));
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
}

khir::Value ColumnIndexBucket::Get() const { return value_; }

template <catalog::SqlType S>
std::string_view MemoryColumnIndex<S>::TypeName() {
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
void MemoryColumnIndex<S>::Reset() {
  program_.Call(program_.GetFunction(FreeFnName<S>()),
                {program_.LoadPtr(value_)});
}

template <catalog::SqlType S>
MemoryColumnIndex<S>::MemoryColumnIndex(khir::ProgramBuilder& program,
                                        bool global)
    : program_(program),
      value_(global
                 ? program.Global(
                       false, false,
                       program.PointerType(program.GetOpaqueType(TypeName())),
                       program.NullPtr(program.PointerType(
                           program.GetOpaqueType(TypeName()))))
                 : program_.Alloca(
                       program.PointerType(program.GetOpaqueType(TypeName())))),
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
          program.PointerType(program.GetOpaqueType(TypeName())))),
      get_value_(program.Global(
          false, true, program.GetStructType(ColumnIndexBucketName),
          program.ConstantStruct(
              program.GetStructType(ColumnIndexBucketName),
              {program.NullPtr(program.PointerType(program.I32Type())),
               program.ConstI32(0)}))) {
  program_.StorePtr(value_, v);
}

template <catalog::SqlType S>
khir::Value MemoryColumnIndex<S>::Get() const {
  return program_.LoadPtr(value_);
}

template <catalog::SqlType S>
catalog::SqlType MemoryColumnIndex<S>::Type() const {
  return S;
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
  return program_.PointerCast(Get(), program_.PointerType(program_.I8Type()));
}

template <catalog::SqlType S>
std::unique_ptr<ColumnIndex> MemoryColumnIndex<S>::Regenerate(
    khir::ProgramBuilder& program, void* value) {
  return std::make_unique<MemoryColumnIndex<S>>(
      program, program.PointerCast(
                   program.ConstPtr(value),
                   program.PointerType(program.GetOpaqueType(TypeName()))));
}

template <catalog::SqlType S>
void MemoryColumnIndex<S>::ForwardDeclare(khir::ProgramBuilder& program) {
  std::optional<khir::Type> index_type;
  if constexpr (catalog::SqlType::DATE == S) {
    index_type = program.GetOpaqueType(TypeName());
  } else {
    index_type = program.OpaqueType(TypeName());
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

}  // namespace kush::compile::proxy