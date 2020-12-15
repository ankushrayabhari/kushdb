#pragma once

#include <string>
#include <vector>

namespace kush::compile {

class SchemaValues {
 public:
  SchemaValues(std::vector<std::string> values);
  std::string Get(int idx);

 private:
  std::vector<std::string> values_;
};

}  // namespace kush::compile
