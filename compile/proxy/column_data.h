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
class Iterable {
 public:
  virtual ~Iterable() = default;
  virtual Int32<T> Size() = 0;
  virtual std::unique_ptr<Value<T>> operator[](Int32<T>& idx) = 0;
};

template <typename T, catalog::SqlType S>
class ColumnData : public Iterable<T> {
 public:
  ColumnData(ProgramBuilder<T>& program, std::string_view path);
  virtual ~ColumnData();

  Int32<T> Size() override;
  std::unique_ptr<Value<T>> operator[](Int32<T>& idx) override;

  static void ForwardDeclare(ProgramBuilder<T>& program);

 private:
  ProgramBuilder<T>& program_;
  typename ProgramBuilder<T>::Value* value_;
  typename ProgramBuilder<T>::Value* result_;
};

}  // namespace kush::compile::proxy