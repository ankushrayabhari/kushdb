#include "compilation/types.h"

#include <stdexcept>
#include <string>

#include "plan/sql_type.h"

namespace kush::compile {

std::string SqlTypeToRuntimeType(plan::SqlType type) {
  switch (type) {
    case plan::SqlType::INT:
      return "int32_t";
  }

  throw std::runtime_error("Unknown type");
}

}  // namespace kush::compile
