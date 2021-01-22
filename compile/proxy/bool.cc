#include "compile/proxy/bool.h"

#include "compile/ir_registry.h"
#include "compile/program_builder.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

template <typename T>
Bool<T>::Bool(ProgramBuilder<T>& program,
              typename ProgramBuilder<T>::Value& value)
    : program_(program), value_(value) {}

template <typename T>
Bool<T>::Bool(ProgramBuilder<T>& program, bool value)
    : program_(program),
      value_(value ? program_.ConstI8(1) : program_.ConstI8(0)) {}

template <typename T>
typename ProgramBuilder<T>::Value& Bool<T>::Get() const {
  return value_;
}

template <typename T>
Bool<T> Bool<T>::operator!() {
  return Bool(program_, program_.LNotI8(value_));
}

template <typename T>
Bool<T> Bool<T>::operator==(const Bool<T>& rhs) {
  return Bool(program_,
              program_.CmpI8(T::CompType::ICMP_EQ, value_, rhs.value_));
}

template <typename T>
Bool<T> Bool<T>::operator!=(const Bool<T>& rhs) {
  return Bool(program_,
              program_.CmpI8(T::CompType::ICMP_NE, value_, rhs.value_));
}

INSTANTIATE_ON_IR(Bool);

}  // namespace kush::compile::proxy