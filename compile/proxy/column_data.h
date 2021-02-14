#pragma once

#include <memory>

#include "catalog/sql_type.h"
#include "compile/ir_registry.h"
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
  virtual UInt32<T> Size() = 0;
  virtual std::unique_ptr<Value<T>> operator[](UInt32<T>& idx) = 0;
};

template <typename T>
class ForwardDeclaredColumnDataFunctions {
 public:
  ForwardDeclaredColumnDataFunctions(
      typename ProgramBuilder<T>::Type& struct_type,
      typename ProgramBuilder<T>::Function& open,
      typename ProgramBuilder<T>::Function& close,
      typename ProgramBuilder<T>::Function& get,
      typename ProgramBuilder<T>::Function& size);

  typename ProgramBuilder<T>::Type& StructType();
  typename ProgramBuilder<T>::Function& Open();
  typename ProgramBuilder<T>::Function& Close();
  typename ProgramBuilder<T>::Function& Get();
  typename ProgramBuilder<T>::Function& Size();

 private:
  typename ProgramBuilder<T>::Type& struct_type_;
  typename ProgramBuilder<T>::Function& open_;
  typename ProgramBuilder<T>::Function& close_;
  typename ProgramBuilder<T>::Function& get_;
  typename ProgramBuilder<T>::Function& size_;
};

template <typename T, catalog::SqlType S>
class ColumnData : public Iterable<T> {
 public:
  ColumnData(ProgramBuilder<T>& program,
             ForwardDeclaredColumnDataFunctions<T>& funcs,
             std::string_view path);
  virtual ~ColumnData();

  UInt32<T> Size() override;
  std::unique_ptr<Value<T>> operator[](UInt32<T>& idx) override;

  static ForwardDeclaredColumnDataFunctions<T> ForwardDeclare(
      ProgramBuilder<T>& program);

 private:
  ProgramBuilder<T>& program_;
  typename ProgramBuilder<T>::Value* value_;
  ForwardDeclaredColumnDataFunctions<T>& funcs_;
};

}  // namespace kush::compile::proxy