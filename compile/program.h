#pragma once

namespace kush::compile {

class Program {
 public:
  virtual ~Program() = default;
  virtual void Execute() const = 0;
};

}  // namespace kush::compile