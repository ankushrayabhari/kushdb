#pragma once

#include <memory>

#include "catalog/sql_type.h"
#include "compile/khir/program_builder.h"
#include "compile/proxy/float.h"
#include "compile/proxy/int.h"
#include "compile/proxy/ptr.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

class ColumnIndex {
 public:
  virtual ~ColumnIndex() = default;

  virtual void Insert(const proxy::Value& v, const proxy::Int32& tuple_idx) = 0;
  virtual proxy::Int32 GetNextGreater(const proxy::Value& v,
                                      const proxy::Int32& tuple_idx,
                                      const proxy::Int32& cardinality) = 0;
};

class GlobalColumnIndex {
 public:
  virtual ~GlobalColumnIndex() = default;
  virtual std::unique_ptr<ColumnIndex> Get() = 0;
};

template <catalog::SqlType S>
class GlobalColumnIndexImpl : public GlobalColumnIndex {
 public:
  GlobalColumnIndexImpl(khir::ProgramBuilder& program);
  virtual ~GlobalColumnIndexImpl() = default;

  void Reset();

  std::unique_ptr<ColumnIndex> Get() override;

 private:
  khir::ProgramBuilder& program_;
  std::function<khir::Value()> global_ref_generator_;
};

template <catalog::SqlType S>
class ColumnIndexImpl : public ColumnIndex {
 public:
  ColumnIndexImpl(khir::ProgramBuilder& program, khir::Value value);
  virtual ~ColumnIndexImpl() = default;

  void Insert(const proxy::Value& v, const proxy::Int32& tuple_idx) override;
  proxy::Int32 GetNextGreater(const proxy::Value& v,
                              const proxy::Int32& tuple_idx,
                              const proxy::Int32& cardinality) override;

  static void ForwardDeclare(khir::ProgramBuilder& program);

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
};

}  // namespace kush::compile::proxy