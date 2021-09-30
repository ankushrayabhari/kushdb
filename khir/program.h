#pragma once

#include <string_view>

namespace kush::khir {

class Program {
 public:
  virtual ~Program() = default;

  virtual void Compile() = 0;
  virtual void* GetFunction(std::string_view name) const = 0;
};

}  // namespace kush::khir