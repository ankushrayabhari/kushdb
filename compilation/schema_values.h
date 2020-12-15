#pragma once

#include <string>
#include <vector>

namespace kush::compile {

class SchemaValues {
 public:
  SchemaValues() = default;
  void AddVariable(std::string variable, std::string type);
  std::string Variable(int idx) const;
  std::string Type(int idx) const;

 private:
  std::vector<std::pair<std::string, std::string>> values_;
};

}  // namespace kush::compile
