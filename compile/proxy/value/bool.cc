#include <memory>

#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

Bool::Bool(khir::ProgramBuilder& program, const khir::Value& value)
    : program_(program), value_(value) {}

Bool::Bool(khir::ProgramBuilder& program, bool value)
    : program_(program), value_(program_.ConstI1(value)) {}

Bool& Bool::operator=(const Bool& rhs) {
  value_ = rhs.value_;
  assert(&program_ == &rhs.program_);
  return *this;
}

Bool& Bool::operator=(Bool&& rhs) {
  value_ = rhs.value_;
  assert(&program_ == &rhs.program_);
  return *this;
}

khir::Value Bool::Get() const { return value_; }

Bool Bool::operator!() const { return Bool(program_, program_.LNotI1(value_)); }

Bool Bool::operator==(const Bool& rhs) const {
  return Bool(program_, program_.CmpI1(khir::CompType::EQ, value_, rhs.value_));
}

Bool Bool::operator!=(const Bool& rhs) const {
  return Bool(program_, program_.CmpI1(khir::CompType::NE, value_, rhs.value_));
}

Bool Bool::operator||(const Bool& rhs) const {
  throw std::runtime_error("Unimplemented");
  // return Bool(program_, program_.OrI1(value_, rhs.value_));
}

Bool Bool::operator&&(const Bool& rhs) const {
  throw std::runtime_error("Unimplemented");
  // return Bool(program_, program_.AndI1(value_, rhs.value_));
}

std::unique_ptr<Bool> Bool::ToPointer() const {
  return std::make_unique<Bool>(program_, value_);
}

void Bool::Print(proxy::Printer& printer) const { printer.Print(*this); }

proxy::Int64 Bool::Hash() const {
  return proxy::Int64(program_, program_.I64ZextI1(value_));
}

}  // namespace kush::compile::proxy