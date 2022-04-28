#pragma once

#include <memory>

#include "catalog/sql_type.h"
#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

class ColumnIndexBucket {
 public:
  ColumnIndexBucket(khir::ProgramBuilder& program, khir::Value v);

  Int32 Size();
  Int32 operator[](const Int32& v);
  Int32 FastForwardToStart(const Int32& last_tuple);
  Bool DoesNotExist();
  void Copy(const ColumnIndexBucket& rhs);
  khir::Value Get() const;

  static void ForwardDeclare(khir::ProgramBuilder& program);

  static std::string StructName;

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
};

class ColumnIndexBucketArray {
 public:
  ColumnIndexBucketArray(khir::ProgramBuilder& program, int max_size);

  Int32 Size();
  void PushBack(const ColumnIndexBucket& bucket);
  ColumnIndexBucket Get(Int32 idx);

  void InitSortedIntersection(const Int32& next_tuple);
  Int32 PopulateSortedIntersectionResult(khir::Value result,
                                         int32_t result_max_size);
  proxy::Int32 PopulateSortedIntersectionResult(khir::Value result,
                                                int32_t result_max_size,
                                                khir::Value filters,
                                                proxy::Int32 filters_size);

  void Clear();

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
  khir::Value sorted_intersection_idx_value_;
  khir::Value idx_value_;
  int max_size_;
};

Int32 SortedIntersectionResultGet(khir::ProgramBuilder& program,
                                  khir::Value result, const Int32& idx);

class ColumnIndex {
 public:
  virtual ~ColumnIndex() = default;

  virtual void Init() = 0;
  virtual void Reset() = 0;
  virtual ColumnIndexBucket GetBucket(const IRValue& v) = 0;

  // Serializes the data needed to generate a reference to this index in a
  // different program builder.
  // Returns an i8* typed Value
  virtual khir::Value Serialize() = 0;

  // Regenerates a reference to the index inside program. value's content is
  // the same as the one returned by the above Serialize method.
  virtual std::unique_ptr<ColumnIndex> Regenerate(khir::ProgramBuilder& program,
                                                  void* value) = 0;
};

class ColumnIndexBuilder {
 public:
  virtual ~ColumnIndexBuilder() = default;
  virtual void Insert(const IRValue& v, const Int32& tuple_idx) = 0;
};

}  // namespace kush::compile::proxy