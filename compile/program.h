#pragma once

namespace kush::compile {

class Program {
 public:
  // Compile
  virtual ~Program() = default;
  virtual void Compile() const = 0;
  virtual void Execute() const = 0;
};

}  // namespace kush::compile