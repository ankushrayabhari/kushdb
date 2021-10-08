#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

Float64::Float64(khir::ProgramBuilder& program, const khir::Value& value)
    : program_(program), value_(value) {}

Float64::Float64(khir::ProgramBuilder& program, double value)
    : program_(program), value_(program.ConstF64(value)) {}

Float64::Float64(khir::ProgramBuilder& program, const Int8& v)
    : program_(program), value_(program_.F64ConvI8(v.Get())) {}

Float64::Float64(khir::ProgramBuilder& program, const Int16& v)
    : program_(program), value_(program_.F64ConvI16(v.Get())) {}

Float64::Float64(khir::ProgramBuilder& program, const Int32& v)
    : program_(program), value_(program_.F64ConvI32(v.Get())) {}

Float64::Float64(khir::ProgramBuilder& program, const Int64& v)
    : program_(program), value_(program_.F64ConvI64(v.Get())) {}

Float64& Float64::operator=(const Float64& rhs) {
  value_ = rhs.value_;
  assert(&program_ == &rhs.program_);
  return *this;
}

Float64& Float64::operator=(Float64&& rhs) {
  value_ = rhs.value_;
  assert(&program_ == &rhs.program_);
  return *this;
}

khir::Value Float64::Get() const { return value_; }

Float64 Float64::operator+(const Float64& rhs) const {
  return Float64(program_, program_.AddF64(value_, rhs.value_));
}

Float64 Float64::operator+(double rhs) const {
  return Float64(program_, program_.AddF64(value_, program_.ConstF64(rhs)));
}

Float64 Float64::operator-(const Float64& rhs) const {
  return Float64(program_, program_.SubF64(value_, rhs.value_));
}

Float64 Float64::operator-(double rhs) const {
  return Float64(program_, program_.SubF64(value_, program_.ConstF64(rhs)));
}

Float64 Float64::operator*(const Float64& rhs) const {
  return Float64(program_, program_.MulF64(value_, rhs.value_));
}

Float64 Float64::operator*(double rhs) const {
  return Float64(program_, program_.MulF64(value_, program_.ConstF64(rhs)));
}

Float64 Float64::operator/(const Float64& rhs) const {
  return Float64(program_, program_.DivF64(value_, rhs.value_));
}

Float64 Float64::operator/(double rhs) const {
  return Float64(program_, program_.DivF64(value_, program_.ConstF64(rhs)));
}

Bool Float64::operator==(const Float64& rhs) const {
  return Bool(program_,
              program_.CmpF64(khir::CompType::EQ, value_, rhs.value_));
}

Bool Float64::operator==(double rhs) const {
  return Bool(program_, program_.CmpF64(khir::CompType::EQ, value_,
                                        program_.ConstF64(rhs)));
}

Bool Float64::operator!=(const Float64& rhs) const {
  return Bool(program_,
              program_.CmpF64(khir::CompType::NE, value_, rhs.value_));
}

Bool Float64::operator!=(double rhs) const {
  return Bool(program_, program_.CmpF64(khir::CompType::NE, value_,
                                        program_.ConstF64(rhs)));
}

Bool Float64::operator<(const Float64& rhs) const {
  return Bool(program_,
              program_.CmpF64(khir::CompType::LT, value_, rhs.value_));
}

Bool Float64::operator<(double rhs) const {
  return Bool(program_, program_.CmpF64(khir::CompType::LT, value_,
                                        program_.ConstF64(rhs)));
}

Bool Float64::operator<=(const Float64& rhs) const {
  return Bool(program_,
              program_.CmpF64(khir::CompType::LE, value_, rhs.value_));
}

Bool Float64::operator<=(double rhs) const {
  return Bool(program_, program_.CmpF64(khir::CompType::LE, value_,
                                        program_.ConstF64(rhs)));
}

Bool Float64::operator>(const Float64& rhs) const {
  return Bool(program_,
              program_.CmpF64(khir::CompType::GT, value_, rhs.value_));
}

Bool Float64::operator>(double rhs) const {
  return Bool(program_, program_.CmpF64(khir::CompType::GT, value_,
                                        program_.ConstF64(rhs)));
}

Bool Float64::operator>=(const Float64& rhs) const {
  return Bool(program_,
              program_.CmpF64(khir::CompType::GE, value_, rhs.value_));
}

Bool Float64::operator>=(double rhs) const {
  return Bool(program_, program_.CmpF64(khir::CompType::GE, value_,
                                        program_.ConstF64(rhs)));
}

std::unique_ptr<Float64> Float64::ToPointer() const {
  return std::make_unique<Float64>(program_, value_);
}

void Float64::Print(Printer& printer) const { printer.Print(*this); }

Int64 Float64::Hash() const {
  return Int64(program_, program_.I64ConvF64(value_));
}

khir::ProgramBuilder& Float64::ProgramBuilder() const { return program_; }

}  // namespace kush::compile::proxy