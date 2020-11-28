#pragma once

#include <cstdint>
#include <variant>

#include "compilation/cpp_program.h"

namespace kush {
namespace compile {

class Bool {
 public:
  Bool(CppProgram& program, bool value);
  Bool(CppProgram& program, std::string var_);
  Bool operator&&(const Bool& rhs);
  Bool operator||(const Bool& rhs);
  Bool& operator=(const Int32& rhs);

 private:
  std::variant<bool, std::string> value_;
  CppProgram& program_;
};

class Int32 {
 public:
  Int32(CppProgram& program, int32_t value);
  Int32(CppProgram& program, std::string var_);
  Int32 operator+(const Int32& rhs);
  Int32 operator<(const Int32& rhs);
  Int32& operator=(const Int32& rhs);

 private:
  std::variant<int32_t, std::string> value_;
  CppProgram& program_;
};

}  // namespace compile
}  // namespace kush