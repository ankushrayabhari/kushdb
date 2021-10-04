#include "compile/proxy/value/sql_value.h"

#include "compile/proxy/value/ir_value.h"

namespace kush::compile::proxy {

SQLValue::SQLValue(const Int8& value, const Bool& null)
    : value_(value.ToPointer()),
      null_(null),
      type_(catalog::SqlType::BOOLEAN) {}

SQLValue::SQLValue(const Int16& value, const Bool& null)
    : value_(value.ToPointer()),
      null_(null),
      type_(catalog::SqlType::SMALLINT) {}

SQLValue::SQLValue(const Int32& value, const Bool& null)
    : value_(value.ToPointer()), null_(null), type_(catalog::SqlType::INT) {}

SQLValue::SQLValue(const Int64& value, const Bool& null)
    : value_(value.ToPointer()), null_(null), type_(catalog::SqlType::BIGINT) {}

SQLValue::SQLValue(const Float64& value, const Bool& null)
    : value_(value.ToPointer()), null_(null), type_(catalog::SqlType::REAL) {}

SQLValue::SQLValue(const String& value, const Bool& null)
    : value_(value.ToPointer()), null_(null), type_(catalog::SqlType::TEXT) {}

Bool SQLValue::IsNull() { return null_; }

IRValue& SQLValue::Get() { return *value_; }

catalog::SqlType SQLValue::Type() { return type_; }

}  // namespace kush::compile::proxy