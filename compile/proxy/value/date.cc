#include <cstdint>
#include <memory>

#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

Date::Date(khir::ProgramBuilder& program, const khir::Value& value)
    : program_(program), value_(value) {}

int64_t ToUnixMillis(absl::CivilDay value) {
  absl::TimeZone utc = absl::UTCTimeZone();
  absl::Time time = absl::FromCivil(value, utc);
  return absl::ToUnixMillis(time);
}

Date::Date(khir::ProgramBuilder& program, absl::CivilDay value)
    : program_(program), value_(program_.ConstI64(ToUnixMillis(value))) {}

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
              program_.CmpI64(khir::CompType::EQ, value_, rhs.value_));
}

Bool Date::operator!=(const Date& rhs) const {
  return Bool(program_,
              program_.CmpI64(khir::CompType::NE, value_, rhs.value_));
}

Bool Date::operator<(const Date& rhs) const {
  return Bool(program_,
              program_.CmpI64(khir::CompType::LT, value_, rhs.value_));
}

Bool Date::operator<=(const Date& rhs) const {
  return Bool(program_,
              program_.CmpI64(khir::CompType::LE, value_, rhs.value_));
}

Bool Date::operator>(const Date& rhs) const {
  return Bool(program_,
              program_.CmpI64(khir::CompType::GT, value_, rhs.value_));
}

Bool Date::operator>=(const Date& rhs) const {
  return Bool(program_,
              program_.CmpI64(khir::CompType::GE, value_, rhs.value_));
}

std::unique_ptr<Date> Date::ToPointer() const {
  return std::make_unique<Date>(program_, value_);
}

void Date::Print(Printer& printer) const { printer.Print(*this); }

Int64 Date::Hash() const { return Int64(program_, value_); }

khir::ProgramBuilder& Date::ProgramBuilder() const { return program_; }

}  // namespace kush::compile::proxy