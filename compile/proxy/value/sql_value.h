#pragma once

#include "catalog/sql_type.h"
#include "compile/proxy/value/ir_value.h"

namespace kush::compile::proxy {

class SQLValue {
 public:
  SQLValue(std::unique_ptr<IRValue> value, const Bool& null,
           catalog::SqlType type);

  Bool IsNull();
  IRValue& Get();
  catalog::SqlType Type();

 private:
  std::unique_ptr<IRValue> value_;
  Bool null_;
  catalog::SqlType type_;
};

}  // namespace kush::compile::proxy