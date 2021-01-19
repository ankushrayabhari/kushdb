#pragma once

#include "compile/program_builder.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

template <typename T>
class Float64 : public Value<T> {
 public:
  Float64(ProgramBuilder<T>& program, typename ProgramBuilder<T>::Value& value);
  Float64(ProgramBuilder<T>& program, double value);

  typename ProgramBuilder<T>::Value& Get() const override;

  Float64<T> operator+(const Float64<T>& rhs);
  Float64<T> operator-(const Float64<T>& rhs);
  Float64<T> operator*(const Float64<T>& rhs);
  Float64<T> operator/(const Float64<T>& rhs);
  Boolean<T> operator==(const Float64<T>& rhs);
  Boolean<T> operator!=(const Float64<T>& rhs);
  Boolean<T> operator<(const Float64<T>& rhs);
  Boolean<T> operator<=(const Float64<T>& rhs);
  Boolean<T> operator>(const Float64<T>& rhs);
  Boolean<T> operator>=(const Float64<T>& rhs);

 private:
  ProgramBuilder<T>& program_;
  ProgramBuilder<T>::Value& value_;
};

}  // namespace kush::compile::proxy