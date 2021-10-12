#pragma once

#include <memory>

#include "catalog/sql_type.h"
#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

class ColumnIndexBucket {
 public:
  ColumnIndexBucket(khir::ProgramBuilder& program);
  ColumnIndexBucket(khir::ProgramBuilder& program, khir::Value v);

  Int32 Size();
  Int32 operator[](const Int32& v);
  Int32 FastForwardToStart(const Int32& last_tuple);
  Bool DoesNotExist();
  void Copy(const ColumnIndexBucket& rhs);
  khir::Value Get() const;

  static void ForwardDeclare(khir::ProgramBuilder& program);

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
  ColumnIndexBucket GetBucket(const IRValue& v) override;
  khir::Value Serialize() override;
  std::unique_ptr<ColumnIndex> Regenerate(khir::ProgramBuilder& program,
                                          void* value) override;

  static void ForwardDeclare(khir::ProgramBuilder& program);

  static std::string_view TypeName();

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
  khir::Value get_value_;
};

}  // namespace kush::compile::proxy