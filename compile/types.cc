#include "compile/types.h"

#include <stdexcept>
#include <string>

#include "catalog/sql_type.h"

namespace kush::compile {

std::string SqlTypeToRuntimeType(catalog::SqlType type) {
  switch (type) {
    case catalog::SqlType::SMALLINT:
      return "int16_t";
    case catalog::SqlType::INT:
    case catalog::SqlType::DATE:
      return "int32_t";
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

}  // namespace kush::compile
