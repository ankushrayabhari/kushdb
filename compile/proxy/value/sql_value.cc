#include "compile/proxy/value/sql_value.h"

namespace kush::compile::proxy {

SQLValue::SQLValue(std::unique_ptr<IRValue> value, const Bool& null,
                   catalog::SqlType type)
    : value_(std::move(value)), null_(null), type_(type) {}

Bool SQLValue::IsNull() { return null_; }

IRValue& SQLValue::Get() { return *value_; }

catalog::SqlType SQLValue::Type() { return type_; }

}  // namespace kush::compile::proxy