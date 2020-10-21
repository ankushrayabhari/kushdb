#include "util/print_util.h"

#include <iostream>

namespace kush {
namespace util {

std::ostream& Indent(std::ostream& out, int num_indent) {
  for (int i = 0; i < num_indent; i++) {
    out << "  ";
  }
  return out;
}

}  // namespace util
}  // namespace kush
