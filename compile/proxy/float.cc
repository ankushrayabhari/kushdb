#include "compile/proxy/float.h"

#include "compile/ir_registry.h"
#include "compile/program_builder.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

template <typename T>
Float64<T>::Float64(ProgramBuilder<T>& program,
                    typename ProgramBuilder<T>::Value& value)
    : program_(program), value_(value) {}

template <typename T>
Float64<T>::Float64(ProgramBuilder<T>& program, double value)
    : program_(program), value_(program.ConstF64(value)) {}

template <typename T>
typename ProgramBuilder<T>::Value& Float64<T>::Get() const {
  return value_;
}

template <typename T>
Float64<T> Float64<T>::operator+(const Float64<T>& rhs) {
  return Float64(program_, program_.AddF64(value_, rhs.value_));
}

template <typename T>
Float64<T> Float64<T>::operator-(const Float64<T>& rhs) {
  return Float64(program_, program_.SubF64(value_, rhs.value_));
}

template <typename T>
Float64<T> Float64<T>::operator*(const Float64<T>& rhs) {
  return Float64(program_, program_.MulF64(value_, rhs.value_));
}

template <typename T>
Float64<T> Float64<T>::operator/(const Float64<T>& rhs) {
  return Float64(program_, program_.DivF64(value_, rhs.value_));
}

template <typename T>
Bool<T> Float64<T>::operator==(const Float64<T>& rhs) {
  return Bool(program_,
              program_.CmpF64(T::CompType::FCMP_OEQ, value_, rhs.value_));
}

template <typename T>
Bool<T> Float64<T>::operator!=(const Float64<T>& rhs) {
  return Bool(program_,
              program_.CmpF64(T::CompType::FCMP_ONE, value_, rhs.value_));
}

template <typename T>
Bool<T> Float64<T>::operator<(const Float64<T>& rhs) {
  return Bool(program_,
              program_.CmpF64(T::CompType::FCMP_OLT, value_, rhs.value_));
}

template <typename T>
Bool<T> Float64<T>::operator<=(const Float64<T>& rhs) {
  return Bool(program_,
              program_.CmpF64(T::CompType::FCMP_OLE, value_, rhs.value_));
}

template <typename T>
Bool<T> Float64<T>::operator>(const Float64<T>& rhs) {
  return Bool(program_,
              program_.CmpF64(T::CompType::FCMP_OGT, value_, rhs.value_));
}

template <typename T>
Bool<T> Float64<T>::operator>=(const Float64<T>& rhs) {
  return Bool(program_,
              program_.CmpF64(T::CompType::FCMP_OGE, value_, rhs.value_));
}

INSTANTIATE_ON_IR(Float64);

}  // namespace kush::compile::proxy