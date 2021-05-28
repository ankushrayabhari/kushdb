#pragma once

#include <memory>

#include "catalog/sql_type.h"
#include "compile/khir/khir_program_builder.h"
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

template <catalog::SqlType S>
class ColumnIndexImpl : public ColumnIndex {
 public:
  ColumnIndexImpl(khir::KHIRProgramBuilder& program, bool global);
  virtual ~ColumnIndexImpl();

  void Insert(const proxy::Value& v, const proxy::Int32& tuple_idx) override;
  proxy::Int32 GetNextGreater(const proxy::Value& v,
                              const proxy::Int32& tuple_idx,
                              const proxy::Int32& cardinality) override;

  static void ForwardDeclare(khir::KHIRProgramBuilder& program);

 private:
  khir::KHIRProgramBuilder& program_;
  khir::Value value_;
};

}  // namespace kush::compile::proxy