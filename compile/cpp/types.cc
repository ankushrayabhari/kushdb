#include "compile/cpp/types.h"

#include <stdexcept>
#include <string>

#include "catalog/sql_type.h"

namespace kush::compile::cpp {

std::string SqlTypeToRuntimeType(catalog::SqlType type) {
  switch (type) {
    case catalog::SqlType::SMALLINT:
      return "int16_t";
    case catalog::SqlType::INT:
      return "int32_t";
    case catalog::SqlType::DATE:
    case catalog::SqlType::BIGINT:
      return "int64_t";
    case catalog::SqlType::REAL:
      return "double";
    case catalog::SqlType::TEXT:
      return "std::string_view";
    case catalog::SqlType::BOOLEAN:
      return "bool";
  }
}

}  // namespace kush::compile::cpp
