#pragma once

namespace kush::compile {

class Program {
 public:
  virtual ~Program() = default;
  virtual void Compile() = 0;
  virtual void Execute() = 0;
};

}  // namespace kush::compile
