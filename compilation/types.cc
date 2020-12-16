#include "compilation/types.h"

#include <stdexcept>
#include <string>

#include "catalog/sql_type.h"

namespace kush::compile {

std::string SqlTypeToRuntimeType(catalog::SqlType type) {
  switch (type) {
    case catalog::SqlType::SMALLINT:
      return "int16_t";
    case catalog::SqlType::INT:
      return "int32_t";
    case catalog::SqlType::BIGINT:
      return "int64_t";
    case catalog::SqlType::REAL:
      return "float";
  }

  throw std::runtime_error("Unknown type");
}

}  // namespace kush::compile
