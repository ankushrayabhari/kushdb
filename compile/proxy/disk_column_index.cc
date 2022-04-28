#include "compile/proxy/disk_column_index.h"

#include <iostream>
#include <memory>

#include "catalog/sql_type.h"
#include "compile/proxy/column_index.h"
#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"
#include "runtime/column_index.h"
#include "runtime/column_index_bucket.h"

namespace kush::compile::proxy {

namespace {
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
  } else if constexpr (catalog::SqlType::INT == S ||
                       catalog::SqlType::DATE == S) {
    return "kush::runtime::ColumnIndex::GetInt32";
  } else if constexpr (catalog::SqlType::BIGINT == S) {
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
  } else if constexpr (catalog::SqlType::INT == S ||
                       catalog::SqlType::DATE == S) {
    return reinterpret_cast<void*>(runtime::ColumnIndex::GetInt32);
  } else if constexpr (catalog::SqlType::BIGINT == S) {
    return reinterpret_cast<void*>(runtime::ColumnIndex::GetInt64);
  } else if constexpr (catalog::SqlType::REAL == S) {
    return reinterpret_cast<void*>(runtime::ColumnIndex::GetFloat64);
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return reinterpret_cast<void*>(runtime::ColumnIndex::GetInt8);
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return reinterpret_cast<void*>(runtime::ColumnIndex::GetText);
  }
}
}  // namespace

template <catalog::SqlType S>
DiskColumnIndex<S>::DiskColumnIndex(khir::ProgramBuilder& program,
                                    std::string_view path)
    : program_(program),
      path_(path),
      path_value_(program.GlobalConstCharArray(path)),
      value_(program.Global(
          program.GetStructType(DiskColumnIndexName),
          program.ConstantStruct(
              program.GetStructType(DiskColumnIndexName),
              {program.NullPtr(program.PointerType(
                   program.GetOpaqueType(DiskColumnIndexDataName))),
               program.ConstI64(0)}))),
      get_value_(program.Global(
          program.GetStructType(ColumnIndexBucket::StructName),
          program.ConstantStruct(
              program.GetStructType(ColumnIndexBucket::StructName),
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
          program.GetStructType(ColumnIndexBucket::StructName),
          program.ConstantStruct(
              program.GetStructType(ColumnIndexBucket::StructName),
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
  } else if constexpr (catalog::SqlType::INT == S ||
                       catalog::SqlType::DATE == S) {
    key_type = program.I32Type();
  } else if constexpr (catalog::SqlType::BIGINT == S) {
    key_type = program.I64Type();
  } else if constexpr (catalog::SqlType::REAL == S) {
    key_type = program.F64Type();
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    key_type = program.I1Type();
  } else if constexpr (catalog::SqlType::TEXT == S) {
    key_type =
        program.PointerType(program.GetStructType(String::StringStructName));
  }

  auto bucket_type = program.GetStructType(ColumnIndexBucket::StructName);
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