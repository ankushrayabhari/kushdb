#pragma once

#include <memory>

#include "catalog/sql_type.h"
#include "compile/proxy/float.h"
#include "compile/proxy/int.h"
#include "compile/proxy/value.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

class IndexBucket {
 public:
  IndexBucket(khir::ProgramBuilder& program, khir::Value v);

  proxy::Int32 FastForwardToStart(const proxy::Int32& last_tuple);
  proxy::Int32 Size();
  proxy::Int32 Get(const proxy::Int32& v);

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
  virtual void Insert(const proxy::Value& v, const proxy::Int32& tuple_idx) = 0;
  virtual IndexBucket GetBucket(const proxy::Value& v) = 0;
};

template <catalog::SqlType S>
class ColumnIndexImpl : public ColumnIndex {
 public:
  ColumnIndexImpl(khir::ProgramBuilder& program, bool global);
  ColumnIndexImpl(khir::ProgramBuilder& program, khir::Value v);
  virtual ~ColumnIndexImpl() = default;

  void Reset() override;
  void Insert(const proxy::Value& v, const proxy::Int32& tuple_idx) override;
  khir::Value Get() const override;
  catalog::SqlType Type() const override;
  IndexBucket GetBucket(const proxy::Value& v) override;

  static void ForwardDeclare(khir::ProgramBuilder& program);

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
};

}  // namespace kush::compile::proxy