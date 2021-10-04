#pragma once

#include "catalog/sql_type.h"
#include "compile/proxy/value/ir_value.h"

namespace kush::compile::proxy {

class SQLValue {
 public:
  SQLValue(const Bool& value, const Bool& null);
  SQLValue(const Int16& value, const Bool& null);
  SQLValue(const Int32& value, const Bool& null);
  SQLValue(const Int64& value, const Bool& null);
  SQLValue(const Float64& value, const Bool& null);
  SQLValue(const String& value, const Bool& null);
  SQLValue(SQLValue&&);
  SQLValue& operator=(SQLValue&&);

  Bool IsNull();
  IRValue& Get();
  catalog::SqlType Type();

 private:
  std::unique_ptr<IRValue> value_;
  Bool null_;
  catalog::SqlType type_;
};

}  // namespace kush::compile::proxy