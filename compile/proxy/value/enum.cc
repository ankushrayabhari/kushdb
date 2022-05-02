#include "runtime/enum.h"

#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

namespace {
constexpr std::string_view GetKeyFnName("kush::runtime::Enum::GetKey");
}  // namespace

Enum::Enum(khir::ProgramBuilder& program, int32_t enum_id, int32_t value)
    : program_(program), enum_id_(enum_id), value_(program.ConstI32(value)) {}

Enum::Enum(khir::ProgramBuilder& program, int32_t enum_id,
           const khir::Value& value)
    : program_(program), enum_id_(enum_id), value_(value) {}

Enum& Enum::operator=(const Enum& rhs) {
  enum_id_ = rhs.enum_id_;
  value_ = rhs.value_;
  assert(&program_ == &rhs.program_);
  return *this;
}

Enum& Enum::operator=(Enum&& rhs) {
  enum_id_ = rhs.enum_id_;
  value_ = rhs.value_;
  assert(&program_ == &rhs.program_);
  return *this;
}

Bool Enum::operator==(const Enum& rhs) const {
  if (rhs.enum_id_ != enum_id_) {
    throw std::runtime_error("Comparing two different enums.");
  }

  return Bool(program_,
              program_.CmpI32(khir::CompType::EQ, value_, rhs.value_));
}

Bool Enum::operator!=(const Enum& rhs) const {
  if (rhs.enum_id_ != enum_id_) {
    throw std::runtime_error("Comparing two different enums.");
  }

  return Bool(program_,
              program_.CmpI32(khir::CompType::NE, value_, rhs.value_));
}

String Enum::ToString() const {
  auto dest = String::Global(program_, "");
  program_.Call(program_.GetFunction(GetKeyFnName),
                {program_.ConstI32(enum_id_), value_, dest.Get()});
  return dest;
}

Int64 Enum::Hash() const {
  return Int64(program_, program_.I64ZextI32(value_));
}

khir::Value Enum::Get() const { return value_; }

int32_t Enum::EnumId() const { return enum_id_; }

void Enum::Print(Printer& printer) const { printer.Print(ToString()); }

std::unique_ptr<Enum> Enum::ToPointer() const {
  return std::make_unique<Enum>(program_, enum_id_, value_);
}

khir::ProgramBuilder& Enum::ProgramBuilder() const { return program_; }

void Enum::ForwardDeclare(khir::ProgramBuilder& program) {
  program.DeclareExternalFunction(
      GetKeyFnName, program.VoidType(),
      {program.I32Type(), program.I32Type(),
       program.PointerType(program.GetStructType(String::StringStructName))},
      reinterpret_cast<void*>(&runtime::Enum::GetKey));
}

}  // namespace kush::compile::proxy