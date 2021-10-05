#include "compile/proxy/value/sql_value.h"

#include "compile/proxy/value/ir_value.h"

namespace kush::compile::proxy {

SQLValue::SQLValue(const Bool& value, const Bool& null)
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

std::unique_ptr<IRValue> CopyIRValue(catalog::SqlType t, IRValue& v) {
  switch (t) {
    case catalog::SqlType::BOOLEAN:
      return dynamic_cast<Bool&>(v).ToPointer();
    case catalog::SqlType::SMALLINT:
      return dynamic_cast<Int16&>(v).ToPointer();
    case catalog::SqlType::INT:
      return dynamic_cast<Int32&>(v).ToPointer();
    case catalog::SqlType::BIGINT:
    case catalog::SqlType::DATE:
      return dynamic_cast<Int64&>(v).ToPointer();
    case catalog::SqlType::REAL:
      return dynamic_cast<Float64&>(v).ToPointer();
    case catalog::SqlType::TEXT:
      return dynamic_cast<String&>(v).ToPointer();
  }
}

SQLValue::SQLValue(const SQLValue& rhs)
    : value_(CopyIRValue(rhs.type_, *rhs.value_)),
      null_(rhs.null_),
      type_(rhs.type_) {}

SQLValue::SQLValue(SQLValue&& rhs)
    : value_(std::move(rhs.value_)),
      null_(std::move(rhs.null_)),
      type_(rhs.type_) {}

SQLValue& SQLValue::operator=(const SQLValue& rhs) {
  value_ = CopyIRValue(rhs.type_, *rhs.value_);
  null_ = rhs.null_;
  type_ = rhs.type_;
  return *this;
}

SQLValue& SQLValue::operator=(SQLValue&& rhs) {
  value_ = std::move(rhs.value_);
  null_ = std::move(rhs.null_);
  type_ = rhs.type_;
  return *this;
}

Bool SQLValue::IsNull() const { return null_; }

IRValue& SQLValue::Get() const { return *value_; }

catalog::SqlType SQLValue::Type() const { return type_; }

}  // namespace kush::compile::proxy