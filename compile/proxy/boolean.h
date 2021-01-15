#pragma once

#include <string>
#include <string_view>

#include "compile/llvm/llvm_program.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

class Boolean : public Value {
 public:
  explicit Boolean(CppProgram& program);
  Boolean(CppProgram& program, bool value);

  std::string_view Get() const override;

  void Assign(Boolean& rhs);
  std::unique_ptr<Boolean> operator!();
  std::unique_ptr<Boolean> operator&&(Boolean& rhs);
  std::unique_ptr<Boolean> operator||(Boolean& rhs);
  std::unique_ptr<Boolean> operator==(Boolean& rhs);
  std::unique_ptr<Boolean> operator!=(Boolean& rhs);

 private:
  CppProgram& program_;
  std::string variable_;
};

}  // namespace kush::compile::proxy