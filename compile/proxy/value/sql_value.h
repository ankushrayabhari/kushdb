#pragma once

#include "catalog/sql_type.h"
#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"
#include "plan/expression/binary_arithmetic_expression.h"

namespace kush::compile::proxy {

class SQLValue {
 public:
  SQLValue(const Bool& value, const Bool& null);
  SQLValue(const Int16& value, const Bool& null);
  SQLValue(const Int32& value, const Bool& null);
  SQLValue(const Int64& value, const Bool& null);
  SQLValue(const Float64& value, const Bool& null);
  SQLValue(const String& value, const Bool& null);
  SQLValue(std::unique_ptr<IRValue> value, catalog::SqlType type,
           const Bool& null);
  SQLValue(const SQLValue&);
  SQLValue(SQLValue&&);
  SQLValue& operator=(SQLValue&&);
  SQLValue& operator=(const SQLValue&);

  Bool IsNull() const;
  IRValue& Get() const;
  catalog::SqlType Type() const;
  khir::ProgramBuilder& ProgramBuilder() const;

 private:
  khir::ProgramBuilder& program_;
  std::unique_ptr<IRValue> value_;
  Bool null_;
  catalog::SqlType type_;
};

}  // namespace kush::compile::proxy