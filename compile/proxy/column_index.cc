#include "compile/proxy/column_index.h"

#include <iostream>
#include <memory>

#include "catalog/sql_type.h"
#include "compile/proxy/float.h"
#include "compile/proxy/int.h"
#include "compile/proxy/string.h"
#include "compile/proxy/value.h"
#include "khir/program_builder.h"
#include "runtime/column_index.h"

namespace kush::compile::proxy {

template <catalog::SqlType S>
std::string_view CreateFnName() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return "_ZN4kush7runtime11ColumnIndex16CreateInt16IndexEv";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "_ZN4kush7runtime11ColumnIndex16CreateInt32IndexEv";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "_ZN4kush7runtime11ColumnIndex16CreateInt64IndexEv";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "_ZN4kush7runtime11ColumnIndex18CreateFloat64IndexEv";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "_ZN4kush7runtime11ColumnIndex15CreateInt8IndexEv";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "_ZN4kush7runtime11ColumnIndex15CreateTextIndexB5cxx11Ev";
  }
}

template <catalog::SqlType S>
void* CreateFn() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::CreateInt16Index);
  } else if constexpr (catalog::SqlType::INT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::CreateInt32Index);
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::CreateInt64Index);
  } else if constexpr (catalog::SqlType::REAL == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::CreateFloat64Index);
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::CreateInt8Index);
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::CreateTextIndex);
  }
}

template <catalog::SqlType S>
std::string_view FreeFnName() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return "_ZN4kush7runtime11ColumnIndex14FreeInt16IndexEPN4absl13flat_hash_"
           "mapIsSt6vectorIiSaIiEENS2_13hash_internal4HashIsEESt8equal_"
           "toIsESaISt4pairIKsS6_EEEE";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "_ZN4kush7runtime11ColumnIndex14FreeInt32IndexEPN4absl13flat_hash_"
           "mapIiSt6vectorIiSaIiEENS2_13hash_internal4HashIiEESt8equal_"
           "toIiESaISt4pairIKiS6_EEEE";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "_ZN4kush7runtime11ColumnIndex14FreeInt64IndexEPN4absl13flat_hash_"
           "mapIlSt6vectorIiSaIiEENS2_13hash_internal4HashIlEESt8equal_"
           "toIlESaISt4pairIKlS6_EEEE";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "_ZN4kush7runtime11ColumnIndex16FreeFloat64IndexEPN4absl13flat_hash_"
           "mapIdSt6vectorIiSaIiEENS2_13hash_internal4HashIdEESt8equal_"
           "toIdESaISt4pairIKdS6_EEEE";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "_ZN4kush7runtime11ColumnIndex13FreeInt8IndexEPN4absl13flat_hash_"
           "mapIaSt6vectorIiSaIiEENS2_13hash_internal4HashIaEESt8equal_"
           "toIaESaISt4pairIKaS6_EEEE";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "_ZN4kush7runtime11ColumnIndex13FreeTextIndexEPN4absl13flat_hash_"
           "mapINSt7__cxx1112basic_stringIcSt11char_"
           "traitsIcESaIcEEESt6vectorIiSaIiEENS2_18container_"
           "internal10StringHashENSD_12StringHashEq2EqESaISt4pairIKS9_SC_EEEE";
  }
}

template <catalog::SqlType S>
void* FreeFn() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::FreeInt16Index);
  } else if constexpr (catalog::SqlType::INT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::FreeInt32Index);
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::FreeInt64Index);
  } else if constexpr (catalog::SqlType::REAL == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::FreeFloat64Index);
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::FreeInt8Index);
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::FreeTextIndex);
  }
}

template <catalog::SqlType S>
std::string_view GetBucketFnName() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return "_ZN4kush7runtime11ColumnIndex19GetBucketInt16IndexEPN4absl13flat_"
           "hash_mapIsSt6vectorIiSaIiEENS2_13hash_internal4HashIsEESt8equal_"
           "toIsESaISt4pairIKsS6_EEEEs";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "_ZN4kush7runtime11ColumnIndex19GetBucketInt32IndexEPN4absl13flat_"
           "hash_mapIiSt6vectorIiSaIiEENS2_13hash_internal4HashIiEESt8equal_"
           "toIiESaISt4pairIKiS6_EEEEi";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "_ZN4kush7runtime11ColumnIndex19GetBucketInt64IndexEPN4absl13flat_"
           "hash_mapIlSt6vectorIiSaIiEENS2_13hash_internal4HashIlEESt8equal_"
           "toIlESaISt4pairIKlS6_EEEEl";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "_ZN4kush7runtime11ColumnIndex21GetBucketFloat64IndexEPN4absl13flat_"
           "hash_mapIdSt6vectorIiSaIiEENS2_13hash_internal4HashIdEESt8equal_"
           "toIdESaISt4pairIKdS6_EEEEd";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "_ZN4kush7runtime11ColumnIndex18GetBucketInt8IndexEPN4absl13flat_"
           "hash_mapIaSt6vectorIiSaIiEENS2_13hash_internal4HashIaEESt8equal_"
           "toIaESaISt4pairIKaS6_EEEEa";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "_ZN4kush7runtime11ColumnIndex18GetBucketTextIndexEPN4absl13flat_"
           "hash_mapINSt7__cxx1112basic_stringIcSt11char_"
           "traitsIcESaIcEEESt6vectorIiSaIiEENS2_18container_"
           "internal10StringHashENSD_12StringHashEq2EqESaISt4pairIKS9_SC_"
           "EEEEPNS0_6String6StringE";
  }
}

template <catalog::SqlType S>
void* GetBucketFn() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::GetBucketInt16Index);
  } else if constexpr (catalog::SqlType::INT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::GetBucketInt32Index);
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::GetBucketInt64Index);
  } else if constexpr (catalog::SqlType::REAL == S) {
    return reinterpret_cast<void*>(
        &runtime::ColumnIndex::GetBucketFloat64Index);
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::GetBucketInt8Index);
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::GetBucketTextIndex);
  }
}

constexpr std::string_view FastForwardBucketName(
    "_ZN4kush7runtime11ColumnIndex17FastForwardBucketEPSt6vectorIiSaIiEEi");

constexpr std::string_view BucketSizeName(
    "_ZN4kush7runtime11ColumnIndex10BucketSizeEPSt6vectorIiSaIiEE");

constexpr std::string_view BucketGetName(
    "_ZN4kush7runtime11ColumnIndex9BucketGetEPSt6vectorIiSaIiEEi");

template <catalog::SqlType S>
std::string_view InsertFnName() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return "_ZN4kush7runtime11ColumnIndex16InsertInt16IndexEPN4absl13flat_hash_"
           "mapIsSt6vectorIiSaIiEENS2_13hash_internal4HashIsEESt8equal_"
           "toIsESaISt4pairIKsS6_EEEEsi";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "_ZN4kush7runtime11ColumnIndex16InsertInt32IndexEPN4absl13flat_hash_"
           "mapIiSt6vectorIiSaIiEENS2_13hash_internal4HashIiEESt8equal_"
           "toIiESaISt4pairIKiS6_EEEEii";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "_ZN4kush7runtime11ColumnIndex16InsertInt64IndexEPN4absl13flat_hash_"
           "mapIlSt6vectorIiSaIiEENS2_13hash_internal4HashIlEESt8equal_"
           "toIlESaISt4pairIKlS6_EEEEli";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "_ZN4kush7runtime11ColumnIndex18InsertFloat64IndexEPN4absl13flat_"
           "hash_mapIdSt6vectorIiSaIiEENS2_13hash_internal4HashIdEESt8equal_"
           "toIdESaISt4pairIKdS6_EEEEdi";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "_ZN4kush7runtime11ColumnIndex15InsertInt8IndexEPN4absl13flat_hash_"
           "mapIaSt6vectorIiSaIiEENS2_13hash_internal4HashIaEESt8equal_"
           "toIaESaISt4pairIKaS6_EEEEai";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "_ZN4kush7runtime11ColumnIndex15InsertTextIndexEPN4absl13flat_hash_"
           "mapINSt7__cxx1112basic_stringIcSt11char_"
           "traitsIcESaIcEEESt6vectorIiSaIiEENS2_18container_"
           "internal10StringHashENSD_12StringHashEq2EqESaISt4pairIKS9_SC_"
           "EEEEPNS0_6String6StringEi";
  }
}

template <catalog::SqlType S>
void* InsertFn() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::InsertInt16Index);
  } else if constexpr (catalog::SqlType::INT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::InsertInt32Index);
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::InsertInt64Index);
  } else if constexpr (catalog::SqlType::REAL == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::InsertFloat64Index);
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::InsertInt8Index);
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::InsertTextIndex);
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
      value_(global
                 ? program.Global(
                       false, false, program.PointerType(program.I8Type()),
                       program.NullPtr(program.PointerType(program.I8Type())))
                 : program_.Alloca(program.PointerType(program.I8Type()))) {
  program_.StorePtr(value_,
                    program_.Call(program_.GetFunction(CreateFnName<S>()), {}));
}

template <catalog::SqlType S>
ColumnIndexImpl<S>::ColumnIndexImpl(khir::ProgramBuilder& program,
                                    khir::Value v)
    : program_(program),
      value_(program_.Alloca(program.PointerType(program.I8Type()))) {
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
void ColumnIndexImpl<S>::Insert(const proxy::Value& v,
                                const proxy::Int32& tuple_idx) {
  program_.Call(program_.GetFunction(InsertFnName<S>()),
                {program_.LoadPtr(value_), v.Get(), tuple_idx.Get()});
}

template <catalog::SqlType S>
IndexBucket ColumnIndexImpl<S>::GetBucket(const proxy::Value& v) {
  return IndexBucket(program_,
                     program_.Call(program_.GetFunction(GetBucketFnName<S>()),
                                   {program_.LoadPtr(value_), v.Get()}));
}

template <catalog::SqlType S>
void ColumnIndexImpl<S>::ForwardDeclare(khir::ProgramBuilder& program) {
  auto index_type = program.I8Type();
  auto index_ptr_type = program.PointerType(index_type);

  auto bucket_type = program.I8Type();
  auto bucket_ptr_type = program.PointerType(bucket_type);

  std::optional<typename khir::Type> value_type;
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
      reinterpret_cast<void*>(&runtime::ColumnIndex::FastForwardBucket));
  program.DeclareExternalFunction(
      BucketSizeName, program.I32Type(), {bucket_ptr_type},
      reinterpret_cast<void*>(&runtime::ColumnIndex::BucketSize));
  program.DeclareExternalFunction(
      BucketGetName, program.I32Type(), {bucket_ptr_type, program.I32Type()},
      reinterpret_cast<void*>(&runtime::ColumnIndex::BucketGet));
}

IndexBucket::IndexBucket(khir::ProgramBuilder& program, khir::Value v)
    : program_(program), value_(v) {}

proxy::Int32 IndexBucket::FastForwardToStart(const proxy::Int32& last_tuple) {
  return proxy::Int32(program_,
                      program_.Call(program_.GetFunction(FastForwardBucketName),
                                    {value_, last_tuple.Get()}));
}

proxy::Int32 IndexBucket::Size() {
  return proxy::Int32(
      program_, program_.Call(program_.GetFunction(BucketSizeName), {value_}));
}

proxy::Int32 IndexBucket::operator[](const proxy::Int32& v) {
  return proxy::Int32(
      program_,
      program_.Call(program_.GetFunction(BucketGetName), {value_, v.Get()}));
}

proxy::Bool IndexBucket::DoesNotExist() {
  return proxy::Bool(program_, program_.IsNullPtr(value_));
}

IndexBucketList::IndexBucketList(khir::ProgramBuilder& program)
    : program_(program) {}

proxy::Int32 IndexBucketList::Size() {
  // TODO: implement this
  return proxy::Int32(program_, 0);
}

void IndexBucketList::PushBack(const IndexBucket& bucket) {
  // TODO: implement this
}

template class ColumnIndexImpl<catalog::SqlType::SMALLINT>;
template class ColumnIndexImpl<catalog::SqlType::INT>;
template class ColumnIndexImpl<catalog::SqlType::BIGINT>;
template class ColumnIndexImpl<catalog::SqlType::REAL>;
template class ColumnIndexImpl<catalog::SqlType::DATE>;
template class ColumnIndexImpl<catalog::SqlType::BOOLEAN>;
template class ColumnIndexImpl<catalog::SqlType::TEXT>;

}  // namespace kush::compile::proxy