#include "compile/proxy/memory_column_index.h"

#include <iostream>
#include <memory>

#include "catalog/sql_type.h"
#include "compile/proxy/column_index.h"
#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"
#include "runtime/column_index_bucket.h"
#include "runtime/memory_column_index.h"

namespace kush::compile::proxy {

namespace {
template <catalog::SqlType S>
std::string_view TypeName() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return "kush::runtime::MemoryColumnIndex::Int16Index";
  } else if constexpr (catalog::SqlType::INT == S ||
                       catalog::SqlType::DATE == S) {
    return "kush::runtime::MemoryColumnIndex::Int32Index";
  } else if constexpr (catalog::SqlType::BIGINT == S) {
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
  } else if constexpr (catalog::SqlType::INT == S ||
                       catalog::SqlType::DATE == S) {
    return "kush::runtime::MemoryColumnIndex::CreateInt32Index";
  } else if constexpr (catalog::SqlType::BIGINT == S) {
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
  } else if constexpr (catalog::SqlType::INT == S ||
                       catalog::SqlType::DATE == S) {
    return reinterpret_cast<void*>(
        &runtime::MemoryColumnIndex::CreateInt32Index);
  } else if constexpr (catalog::SqlType::BIGINT == S) {
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
  } else if constexpr (catalog::SqlType::INT == S ||
                       catalog::SqlType::DATE == S) {
    return "kush::runtime::MemoryColumnIndex::FreeInt32Index";
  } else if constexpr (catalog::SqlType::BIGINT == S) {
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
  } else if constexpr (catalog::SqlType::INT == S ||
                       catalog::SqlType::DATE == S) {
    return reinterpret_cast<void*>(&runtime::MemoryColumnIndex::FreeInt32Index);
  } else if constexpr (catalog::SqlType::BIGINT == S) {
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
  } else if constexpr (catalog::SqlType::INT == S ||
                       catalog::SqlType::DATE == S) {
    return "kush::runtime::MemoryColumnIndex::GetBucketInt32Index";
  } else if constexpr (catalog::SqlType::BIGINT == S) {
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
  } else if constexpr (catalog::SqlType::INT == S ||
                       catalog::SqlType::DATE == S) {
    return reinterpret_cast<void*>(
        &runtime::MemoryColumnIndex::GetBucketInt32Index);
  } else if constexpr (catalog::SqlType::BIGINT == S) {
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
  } else if constexpr (catalog::SqlType::INT == S ||
                       catalog::SqlType::DATE == S) {
    return "kush::runtime::MemoryColumnIndex::InsertInt32Index";
  } else if constexpr (catalog::SqlType::BIGINT == S) {
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
  } else if constexpr (catalog::SqlType::INT == S ||
                       catalog::SqlType::DATE == S) {
    return reinterpret_cast<void*>(
        &runtime::MemoryColumnIndex::InsertInt32Index);
  } else if constexpr (catalog::SqlType::BIGINT == S) {
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
}  // namespace

template <catalog::SqlType S>
void MemoryColumnIndex<S>::Init() {}

template <catalog::SqlType S>
void MemoryColumnIndex<S>::Reset() {
  program_.Call(program_.GetFunction(FreeFnName<S>()),
                {program_.LoadPtr(value_)});
}

template <catalog::SqlType S>
MemoryColumnIndex<S>::MemoryColumnIndex(khir::ProgramBuilder& program)
    : program_(program),
      value_(program.Global(
          program.PointerType(program.GetOpaqueType(TypeName<S>())),
          program.NullPtr(
              program.PointerType(program.GetOpaqueType(TypeName<S>()))))),
      get_value_(program.Global(
          program.GetStructType(ColumnIndexBucket::StructName),
          program.ConstantStruct(
              program.GetStructType(ColumnIndexBucket::StructName),
              {program.NullPtr(program.PointerType(program.I32Type())),
               program.ConstI32(0)}))) {
  program_.StorePtr(value_,
                    program_.Call(program_.GetFunction(CreateFnName<S>()), {}));
}

template <catalog::SqlType S>
MemoryColumnIndex<S>::MemoryColumnIndex(khir::ProgramBuilder& program,
                                        khir::Value v)
    : program_(program),
      value_(program.Global(
          program.PointerType(program.GetOpaqueType(TypeName<S>())), v)),
      get_value_(program.Global(
          program.GetStructType(ColumnIndexBucket::StructName),
          program.ConstantStruct(
              program.GetStructType(ColumnIndexBucket::StructName),
              {program.NullPtr(program.PointerType(program.I32Type())),
               program.ConstI32(0)}))) {}

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
  auto index_type = program.OpaqueType(TypeName<S>());
  auto index_ptr_type = program.PointerType(index_type);

  std::optional<khir::Type> value_type;
  if constexpr (catalog::SqlType::SMALLINT == S) {
    value_type = program.I16Type();
  } else if constexpr (catalog::SqlType::INT == S ||
                       catalog::SqlType::DATE == S) {
    value_type = program.I32Type();
  } else if constexpr (catalog::SqlType::BIGINT == S) {
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
      program.PointerType(program.GetStructType(ColumnIndexBucket::StructName));
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