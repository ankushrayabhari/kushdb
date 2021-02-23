#include "compile/proxy/float.h"

#include "compile/ir_registry.h"
#include "compile/program_builder.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/numeric.h"
#include "compile/proxy/value.h"
#include "plan/expression/binary_arithmetic_expression.h"

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
  return Float64<T>(program_, program_.AddF64(value_, rhs.value_));
}

template <typename T>
Float64<T> Float64<T>::operator-(const Float64<T>& rhs) {
  return Float64<T>(program_, program_.SubF64(value_, rhs.value_));
}

template <typename T>
Float64<T> Float64<T>::operator*(const Float64<T>& rhs) {
  return Float64<T>(program_, program_.MulF64(value_, rhs.value_));
}

template <typename T>
Float64<T> Float64<T>::operator/(const Float64<T>& rhs) {
  return Float64<T>(program_, program_.DivF64(value_, rhs.value_));
}

template <typename T>
Bool<T> Float64<T>::operator==(const Float64<T>& rhs) {
  return Bool<T>(program_,
                 program_.CmpF64(T::CompType::FCMP_OEQ, value_, rhs.value_));
}

template <typename T>
Bool<T> Float64<T>::operator!=(const Float64<T>& rhs) {
  return Bool<T>(program_,
                 program_.CmpF64(T::CompType::FCMP_ONE, value_, rhs.value_));
}

template <typename T>
Bool<T> Float64<T>::operator<(const Float64<T>& rhs) {
  return Bool<T>(program_,
                 program_.CmpF64(T::CompType::FCMP_OLT, value_, rhs.value_));
}

template <typename T>
Bool<T> Float64<T>::operator<=(const Float64<T>& rhs) {
  return Bool<T>(program_,
                 program_.CmpF64(T::CompType::FCMP_OLE, value_, rhs.value_));
}

template <typename T>
Bool<T> Float64<T>::operator>(const Float64<T>& rhs) {
  return Bool<T>(program_,
                 program_.CmpF64(T::CompType::FCMP_OGT, value_, rhs.value_));
}

template <typename T>
Bool<T> Float64<T>::operator>=(const Float64<T>& rhs) {
  return Bool<T>(program_,
                 program_.CmpF64(T::CompType::FCMP_OGE, value_, rhs.value_));
}

template <typename T>
std::unique_ptr<Float64<T>> Float64<T>::ToPointer() {
  return std::make_unique<Float64<T>>(program_, value_);
}

template <typename T>
std::unique_ptr<Value<T>> Float64<T>::EvaluateBinary(
    plan::BinaryArithmeticOperatorType op_type, Value<T>& rhs) {
  return EvaluateBinaryNumeric<Float64<T>, T>(op_type, *this, rhs);
}

template <typename T>
void Float64<T>::Print(proxy::Printer<T>& printer) {
  printer.Print(*this);
}

template <typename T>
typename ProgramBuilder<T>::Value& Float64<T>::Hash() {
  return program_.F64ConversionI64(value_);
}

INSTANTIATE_ON_IR(Float64);

}  // namespace kush::compile::proxy