#pragma once

#include "catalog/sql_type.h"
#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"
#include "plan/expression/arithmetic_expression.h"

namespace kush::compile::proxy {

class SQLValue {
 public:
  SQLValue(const Bool& value, const Bool& null);
  SQLValue(const Int16& value, const Bool& null);
  SQLValue(const Int32& value, const Bool& null);
  SQLValue(const Int64& value, const Bool& null);
  SQLValue(const Date& value, const Bool& null);
  SQLValue(const Float64& value, const Bool& null);
  SQLValue(const String& value, const Bool& null);
  SQLValue(std::unique_ptr<IRValue> value, const catalog::Type& type,
           const Bool& null);
  SQLValue(const SQLValue&);
  SQLValue(SQLValue&&);
  SQLValue& operator=(SQLValue&&);
  SQLValue& operator=(const SQLValue&);

  Bool IsNull() const;
  IRValue& Get() const;
  const catalog::Type& Type() const;
  khir::ProgramBuilder& ProgramBuilder() const;

  SQLValue GetNotNullable() const;

 private:
  khir::ProgramBuilder& program_;
  std::unique_ptr<IRValue> value_;
  Bool null_;
  catalog::Type type_;
};

}  // namespace kush::compile::proxy