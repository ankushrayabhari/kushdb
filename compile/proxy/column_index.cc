#include "compile/proxy/column_index.h"

#include <iostream>
#include <memory>

#include "catalog/sql_type.h"
#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"
#include "runtime/column_index_bucket.h"

namespace kush::compile::proxy {

std::string ColumnIndexBucket::StructName("kush::runtime::ColumnIndexBucket");

namespace {
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
constexpr std::string_view BucketListSortedIntersectionPopulateResultFilterName(
    "kush::runtime::ColumnIndexBucket::"
    "BucketListSortedIntersectionPopulateResultFilter");
constexpr std::string_view BucketListSortedIntersectionResultGetName(
    "kush::runtime::ColumnIndexBucket::BucketListSortedIntersectionResultGet");
}  // namespace

ColumnIndexBucket::ColumnIndexBucket(khir::ProgramBuilder& program,
                                     khir::Value v)
    : program_(program), value_(v) {}

void ColumnIndexBucket::Copy(const ColumnIndexBucket& rhs) {
  auto st = program_.GetStructType(ColumnIndexBucket::StructName);
  program_.StorePtr(
      program_.StaticGEP(st, value_, {0, 0}),
      program_.LoadPtr(program_.StaticGEP(st, rhs.value_, {0, 0})));
  program_.StoreI32(
      program_.StaticGEP(st, value_, {0, 1}),
      program_.LoadI32(program_.StaticGEP(st, rhs.value_, {0, 1})));
}

Int32 ColumnIndexBucket::FastForwardToStart(const Int32& last_tuple) {
  return Int32(program_,
               program_.Call(program_.GetFunction(FastForwardBucketName),
                             {value_, last_tuple.Get()}));
}

Int32 ColumnIndexBucket::Size() {
  auto size_ptr = program_.StaticGEP(
      program_.GetStructType(ColumnIndexBucket::StructName), value_, {0, 1});
  return Int32(program_, program_.LoadI32(size_ptr));
}

Int32 ColumnIndexBucket::operator[](const Int32& v) {
  return Int32(program_,
               program_.Call(program_.GetFunction(ColumnIndexBucketGetName),
                             {value_, v.Get()}));
}

Bool ColumnIndexBucket::DoesNotExist() {
  auto st = program_.GetStructType(ColumnIndexBucket::StructName);
  return Bool(program_, program_.IsNullPtr(program_.LoadPtr(
                            program_.StaticGEP(st, value_, {0, 0}))));
}

void ColumnIndexBucket::ForwardDeclare(khir::ProgramBuilder& program) {
  auto index_bucket_type = program.StructType(
      {program.PointerType(program.I32Type()), program.I32Type()},
      ColumnIndexBucket::StructName);
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
      BucketListSortedIntersectionPopulateResultFilterName, program.I32Type(),
      {index_bucket_ptr_type, program.PointerType(program.I32Type()),
       program.PointerType(program.I32Type()), program.I32Type(),
       program.PointerType(program.I32Type()), program.I32Type()},
      reinterpret_cast<void*>(
          &runtime::BucketListSortedIntersectionPopulateResultFilter));

  program.DeclareExternalFunction(
      BucketListSortedIntersectionResultGetName, program.I32Type(),
      {program.PointerType(program.I32Type()), program.I32Type()},
      reinterpret_cast<void*>(&runtime::BucketListSortedIntersectionResultGet));
}

khir::Value ColumnIndexBucket::Get() const { return value_; }

ColumnIndexBucketArray::ColumnIndexBucketArray(khir::ProgramBuilder& program,
                                               int max_size)
    : program_(program),
      value_(program.Global(
          program.ArrayType(
              program.GetStructType(ColumnIndexBucket::StructName), max_size),
          program.ConstantArray(
              program.ArrayType(
                  program.GetStructType(ColumnIndexBucket::StructName),
                  max_size),
              std::vector<khir::Value>(
                  max_size,
                  program.ConstantStruct(
                      program.GetStructType(ColumnIndexBucket::StructName),
                      {program.NullPtr(program.PointerType(program.I32Type())),
                       program.ConstI32(0)}))))),
      sorted_intersection_idx_value_(program.Global(
          program.ArrayType(program.I32Type(), max_size),
          program.ConstantArray(
              program.ArrayType(program.I32Type(), max_size),
              std::vector<khir::Value>(max_size, program.ConstI32(0))))),
      idx_value_(program_.Global(program_.I32Type(), program.ConstI32(0))),
      max_size_(max_size) {
  program_.StoreI32(idx_value_, program_.ConstI32(0));
}

Int32 ColumnIndexBucketArray::Size() {
  return Int32(program_, program_.LoadI32(idx_value_));
}

ColumnIndexBucket ColumnIndexBucketArray::Get(Int32 idx) {
  return ColumnIndexBucket(
      program_,
      program_.Call(program_.GetFunction(BucketListGetName),
                    {program_.StaticGEP(
                         program_.ArrayType(program_.GetStructType(
                                                ColumnIndexBucket::StructName),
                                            max_size_),
                         value_, {0, 0}),
                     idx.Get()}));
}

void ColumnIndexBucketArray::Clear() {
  program_.StoreI32(idx_value_, program_.ConstI32(0));
}

void ColumnIndexBucketArray::PushBack(const ColumnIndexBucket& bucket) {
  auto idx = Size();
  auto dest = Get(idx);
  dest.Copy(bucket);
  program_.StoreI32(idx_value_, (idx + 1).Get());
}

void ColumnIndexBucketArray::InitSortedIntersection(
    const proxy::Int32& next_tuple) {
  program_.Call(
      program_.GetFunction(BucketListSortedIntersectionInitName),
      {program_.StaticGEP(program_.ArrayType(program_.GetStructType(
                                                 ColumnIndexBucket::StructName),
                                             max_size_),
                          value_, {0, 0}),
       program_.LoadI32(idx_value_),
       program_.StaticGEP(program_.ArrayType(program_.I32Type(), max_size_),
                          sorted_intersection_idx_value_, {0, 0}),
       next_tuple.Get()});
}

proxy::Int32 ColumnIndexBucketArray::PopulateSortedIntersectionResult(
    khir::Value result, int32_t result_max_size) {
  return proxy::Int32(
      program_,
      program_.Call(
          program_.GetFunction(BucketListSortedIntersectionPopulateResultName),
          {program_.StaticGEP(
               program_.ArrayType(
                   program_.GetStructType(ColumnIndexBucket::StructName),
                   max_size_),
               value_, {0, 0}),
           program_.LoadI32(idx_value_),
           program_.StaticGEP(program_.ArrayType(program_.I32Type(), max_size_),
                              sorted_intersection_idx_value_, {0, 0}),
           result, program_.ConstI32(result_max_size)}));
}

proxy::Int32 ColumnIndexBucketArray::PopulateSortedIntersectionResult(
    khir::Value result, int32_t result_max_size, khir::Value filters,
    proxy::Int32 filters_size) {
  return proxy::Int32(
      program_,
      program_.Call(
          program_.GetFunction(
              BucketListSortedIntersectionPopulateResultFilterName),
          {program_.StaticGEP(
               program_.ArrayType(
                   program_.GetStructType(ColumnIndexBucket::StructName),
                   max_size_),
               value_, {0, 0}),
           program_.StaticGEP(program_.ArrayType(program_.I32Type(), max_size_),
                              sorted_intersection_idx_value_, {0, 0}),
           result, program_.ConstI32(result_max_size), filters,
           filters_size.Get()}));
}

proxy::Int32 SortedIntersectionResultGet(khir::ProgramBuilder& program,
                                         khir::Value result,
                                         const proxy::Int32& idx) {
  return proxy::Int32(
      program, program.Call(program.GetFunction(
                                BucketListSortedIntersectionResultGetName),
                            {result, idx.Get()}));
}

}  // namespace kush::compile::proxy