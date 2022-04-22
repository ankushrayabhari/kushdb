#include "runtime/date.h"

#include <cstdint>
#include <memory>

#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

Date::Date(khir::ProgramBuilder& program, const khir::Value& value)
    : program_(program), value_(value) {}

Date::Date(khir::ProgramBuilder& program,
           const runtime::Date::DateBuilder& value)
    : program_(program), value_(program_.ConstI32(value.Build())) {}

Date& Date::operator=(const Date& rhs) {
  value_ = rhs.value_;
  assert(&program_ == &rhs.program_);
  return *this;
}

Date& Date::operator=(Date&& rhs) {
  value_ = rhs.value_;
  assert(&program_ == &rhs.program_);
  return *this;
}

khir::Value Date::Get() const { return value_; }

Bool Date::operator==(const Date& rhs) const {
  return Bool(program_,
              program_.CmpI32(khir::CompType::EQ, value_, rhs.value_));
}

Bool Date::operator!=(const Date& rhs) const {
  return Bool(program_,
              program_.CmpI32(khir::CompType::NE, value_, rhs.value_));
}

Bool Date::operator<(const Date& rhs) const {
  return Bool(program_,
              program_.CmpI32(khir::CompType::LT, value_, rhs.value_));
}

Bool Date::operator<=(const Date& rhs) const {
  return Bool(program_,
              program_.CmpI32(khir::CompType::LE, value_, rhs.value_));
}

Bool Date::operator>(const Date& rhs) const {
  return Bool(program_,
              program_.CmpI32(khir::CompType::GT, value_, rhs.value_));
}

Bool Date::operator>=(const Date& rhs) const {
  return Bool(program_,
              program_.CmpI32(khir::CompType::GE, value_, rhs.value_));
}

std::unique_ptr<Date> Date::ToPointer() const {
  return std::make_unique<Date>(program_, value_);
}

void Date::Print(Printer& printer) const { printer.Print(*this); }

Int64 Date::Hash() const { return Int32(program_, value_).Hash(); }

khir::ProgramBuilder& Date::ProgramBuilder() const { return program_; }

}  // namespace kush::compile::proxy