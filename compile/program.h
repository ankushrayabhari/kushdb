#pragma once

namespace kush::compile {

class Program {
 public:
  virtual ~Program() = default;
  virtual void Execute() = 0;
};

}  // namespace kush::compile