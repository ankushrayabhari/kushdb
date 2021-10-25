#include "parse/expression/expression.h"

#include <string>
#include <string_view>

namespace kush::parse {

void Expression::SetAlias(std::string_view alias) { alias_ = alias; }

}  // namespace kush::parse