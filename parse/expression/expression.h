#pragma once

#include <string>
#include <string_view>

namespace kush::parse {

class Expression {
 public:
  void SetAlias(std::string_view alias);

 private:
  std::string alias_;
};

}  // namespace kush::parse