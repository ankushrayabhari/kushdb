#pragma once

#include <memory>

#include "catalog/sql_type.h"
#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

class IndexBucket {
 public:
  IndexBucket(khir::ProgramBuilder& program, khir::Value v);

  Int32 FastForwardToStart(const Int32& last_tuple);
  Int32 Size();
  Int32 operator[](const Int32& v);
  Bool DoesNotExist();

  khir::Value Get() const;

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
};

class IndexBucketList {
 public:
  IndexBucketList(khir::ProgramBuilder& program);

  Int32 Size();
  IndexBucket operator[](const Int32& idx);
  void PushBack(const IndexBucket& bucket);
  void Reset();

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
};

class ColumnIndex {
 public:
  virtual ~ColumnIndex() = default;

  virtual khir::Value Get() const = 0;
  virtual catalog::SqlType Type() const = 0;
  virtual void Reset() = 0;
  virtual void Insert(const IRValue& v, const Int32& tuple_idx) = 0;
  virtual IndexBucket GetBucket(const IRValue& v) = 0;
};

template <catalog::SqlType S>
class ColumnIndexImpl : public ColumnIndex {
 public:
  ColumnIndexImpl(khir::ProgramBuilder& program, bool global);
  ColumnIndexImpl(khir::ProgramBuilder& program, khir::Value v);
  virtual ~ColumnIndexImpl() = default;

  void Reset() override;
  void Insert(const IRValue& v, const Int32& tuple_idx) override;
  khir::Value Get() const override;
  catalog::SqlType Type() const override;
  IndexBucket GetBucket(const IRValue& v) override;

  static void ForwardDeclare(khir::ProgramBuilder& program);

  static std::string_view TypeName();

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
};

}  // namespace kush::compile::proxy