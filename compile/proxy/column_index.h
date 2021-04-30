#pragma once

#include <memory>

#include "catalog/sql_type.h"
#include "compile/program_builder.h"
#include "compile/proxy/float.h"
#include "compile/proxy/int.h"
#include "compile/proxy/ptr.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

template <typename T>
class ColumnIndex {
 public:
  virtual ~ColumnIndex() = default;

  virtual void Insert(const proxy::Value<T>& v,
                      const proxy::Int32<T>& tuple_idx) = 0;
  virtual proxy::Int32<T> GetNextGreater(
      const proxy::Value<T>& v, const proxy::Int32<T>& tuple_idx,
      const proxy::Int32<T>& cardinality) = 0;
};

template <typename T, catalog::SqlType S>
class ColumnIndexImpl : public ColumnIndex<T> {
 public:
  ColumnIndexImpl(ProgramBuilder<T>& program, bool global);
  virtual ~ColumnIndexImpl();

  void Insert(const proxy::Value<T>& v,
              const proxy::Int32<T>& tuple_idx) override;
  proxy::Int32<T> GetNextGreater(const proxy::Value<T>& v,
                                 const proxy::Int32<T>& tuple_idx,
                                 const proxy::Int32<T>& cardinality) override;

  static void ForwardDeclare(ProgramBuilder<T>& program);

 private:
  ProgramBuilder<T>& program_;
  typename ProgramBuilder<T>::Value* value_;
};

}  // namespace kush::compile::proxy