#include "compilation/types.h"

#include <stdexcept>
#include <string>

#include "catalog/sql_type.h"

namespace kush::compile {

std::string SqlTypeToRuntimeType(catalog::SqlType type) {
  switch (type) {
    case catalog::SqlType::INT:
      return "int32_t";
  }

  throw std::runtime_error("Unknown type");
}

}  // namespace kush::compile
