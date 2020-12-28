#pragma once

#include <string>
#include <vector>

namespace kush::compile::cpp {

class SchemaValues {
 public:
  SchemaValues() = default;
  void AddVariable(std::string variable, std::string type);
  std::string Variable(int idx) const;
  std::string Type(int idx) const;
  const std::vector<std::pair<std::string, std::string>>& Values() const;

 private:
  std::vector<std::pair<std::string, std::string>> values_;
};

}  // namespace kush::compile::cpp
