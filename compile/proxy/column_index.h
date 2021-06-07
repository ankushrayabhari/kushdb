#pragma once

#include <memory>

#include "catalog/sql_type.h"
#include "compile/proxy/float.h"
#include "compile/proxy/int.h"
#include "compile/proxy/value.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

class ColumnIndex {
 public:
  virtual ~ColumnIndex() = default;

  virtual void Reset() = 0;
  virtual void Insert(const proxy::Value& v, const proxy::Int32& tuple_idx) = 0;
  virtual proxy::Int32 GetNextGreater(const proxy::Value& v,
                                      const proxy::Int32& tuple_idx,
                                      const proxy::Int32& cardinality) = 0;
};

template <catalog::SqlType S>
class ColumnIndexImpl : public ColumnIndex {
 public:
  ColumnIndexImpl(khir::ProgramBuilder& program, bool global);
  virtual ~ColumnIndexImpl() = default;

  void Reset() override;
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