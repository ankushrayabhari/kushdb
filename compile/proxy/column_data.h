#pragma once

#include <memory>

#include "catalog/sql_type.h"
#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

class Iterable {
 public:
  virtual ~Iterable() = default;
  virtual Int32 Size() = 0;
  virtual std::unique_ptr<IRValue> operator[](Int32& idx) = 0;
  virtual void Reset() = 0;
};

template <catalog::SqlType S>
class ColumnData : public Iterable {
 public:
  ColumnData(khir::ProgramBuilder& program, std::string_view path);
  virtual ~ColumnData() = default;

  Int32 Size() override;
  std::unique_ptr<IRValue> operator[](Int32& idx) override;
  void Reset() override;

  static void ForwardDeclare(khir::ProgramBuilder& program);

 private:
  khir::ProgramBuilder& program_;
  std::optional<khir::Value> value_;
  std::optional<khir::Value> result_;
};

}  // namespace kush::compile::proxy