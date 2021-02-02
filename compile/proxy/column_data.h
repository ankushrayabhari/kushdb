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
class Iterable : public Value<T> {
 public:
  virtual ~Iterable() = default;
  virtual std::unique_ptr<UInt32<T>> Size() = 0;
  virtual std::unique_ptr<Value<T>> operator[](UInt32<T>& idx) = 0;
};

template <typename T, catalog::SqlType S>
class ColumnData {};

template <typename T>
class ColumnData<T, catalog::SqlType::SMALLINT> : public Iterable<T> {
 public:
  ColumnData(ProgramBuilder<T>& program, std::string_view path);
  virtual ~ColumnData() = default;

  typename ProgramBuilder<T>::Value& Get() const override;
  std::unique_ptr<Value<T>> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type, Value<T>& rhs) override;

  std::unique_ptr<UInt32<T>> Size() override;
  std::unique_ptr<Value<T>> operator[](UInt32<T>& idx) override;
};

template <typename T>
class ColumnData<T, catalog::SqlType::INT> : public Iterable<T> {
 public:
  ColumnData(ProgramBuilder<T>& program, std::string_view path);
  virtual ~ColumnData() = default;

  typename ProgramBuilder<T>::Value& Get() const override;
  std::unique_ptr<Value<T>> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type, Value<T>& rhs) override;

  std::unique_ptr<UInt32<T>> Size() override;
  std::unique_ptr<Value<T>> operator[](UInt32<T>& idx) override;
};

template <typename T>
class ColumnData<T, catalog::SqlType::BIGINT> : public Iterable<T> {
 public:
  ColumnData(ProgramBuilder<T>& program, std::string_view path);
  virtual ~ColumnData() = default;

  typename ProgramBuilder<T>::Value& Get() const override;
  std::unique_ptr<Value<T>> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type, Value<T>& rhs) override;

  std::unique_ptr<UInt32<T>> Size() override;
  std::unique_ptr<Value<T>> operator[](UInt32<T>& idx) override;
};

template <typename T>
class ColumnData<T, catalog::SqlType::REAL> : public Iterable<T> {
 public:
  ColumnData(ProgramBuilder<T>& program, std::string_view path);
  virtual ~ColumnData() = default;

  typename ProgramBuilder<T>::Value& Get() const override;
  std::unique_ptr<Value<T>> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type, Value<T>& rhs) override;

  std::unique_ptr<UInt32<T>> Size() override;
  std::unique_ptr<Value<T>> operator[](UInt32<T>& idx) override;
};

template <typename T>
class ColumnData<T, catalog::SqlType::DATE> : public Iterable<T> {
 public:
  ColumnData(ProgramBuilder<T>& program, std::string_view path);
  virtual ~ColumnData() = default;

  typename ProgramBuilder<T>::Value& Get() const override;
  std::unique_ptr<Value<T>> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type, Value<T>& rhs) override;

  std::unique_ptr<UInt32<T>> Size() override;
  std::unique_ptr<Value<T>> operator[](UInt32<T>& idx) override;
};

template <typename T>
class ColumnData<T, catalog::SqlType::BOOLEAN> : public Iterable<T> {
 public:
  ColumnData(ProgramBuilder<T>& program, std::string_view path);
  virtual ~ColumnData() = default;

  typename ProgramBuilder<T>::Value& Get() const override;
  std::unique_ptr<Value<T>> EvaluateBinary(
      plan::BinaryArithmeticOperatorType op_type, Value<T>& rhs) override;

  std::unique_ptr<UInt32<T>> Size() override;
  std::unique_ptr<Value<T>> operator[](UInt32<T>& idx) override;
};

template <typename T>
class ColumnData<T, catalog::SqlType::TEXT> {};

}  // namespace kush::compile::proxy