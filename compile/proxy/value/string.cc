#include "runtime/string.h"

#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

namespace {
constexpr std::string_view CopyFnName("kush::runtime::String::Copy");
constexpr std::string_view ContainsFnName("kush::runtime::String::Contains");
constexpr std::string_view LikeFnName("kush::runtime::String::Like");
constexpr std::string_view EndsWithFnName("kush::runtime::String::EndsWith");
constexpr std::string_view StartsWithFnName(
    "kush::runtime::String::StartsWith");
constexpr std::string_view EqualsFnName("kush::runtime::String::Equals");
constexpr std::string_view NotEqualsFnName("kush::runtime::String::NotEquals");
constexpr std::string_view LessThanFnName("kush::runtime::String::LessThan");
constexpr std::string_view LessThanEqualsFnName(
    "kush::runtime::String::LessThanEquals");
constexpr std::string_view GreaterThanFnName(
    "kush::runtime::String::GreaterThan");
constexpr std::string_view GreaterThanEqualsFnName(
    "kush::runtime::String::GreaterThanEquals");
constexpr std::string_view HashFnName("kush::runtime::String::Hash");
}  // namespace

String::String(khir::ProgramBuilder& program, const khir::Value& value)
    : program_(program), value_(value) {}

String& String::operator=(const String& rhs) {
  value_ = rhs.value_;
  assert(&program_ == &rhs.program_);
  return *this;
}

String& String::operator=(String&& rhs) {
  value_ = rhs.value_;
  assert(&program_ == &rhs.program_);
  return *this;
}

void String::Copy(const String& rhs) const {
  program_.Call(program_.GetFunction(CopyFnName), {value_, rhs.Get()});
}

Bool String::Contains(const String& rhs) const {
  return Bool(program_, program_.Call(program_.GetFunction(ContainsFnName),
                                      {value_, rhs.Get()}));
}

Bool String::StartsWith(const String& rhs) const {
  return Bool(program_, program_.Call(program_.GetFunction(StartsWithFnName),
                                      {value_, rhs.Get()}));
}

Bool String::EndsWith(const String& rhs) const {
  return Bool(program_, program_.Call(program_.GetFunction(EndsWithFnName),
                                      {value_, rhs.Get()}));
}

Bool String::Like(re2::RE2* rhs) const {
  return Bool(program_, program_.Call(program_.GetFunction(LikeFnName),
                                      {value_, program_.ConstPtr(rhs)}));
}

Bool String::operator==(const String& rhs) const {
  return Bool(program_, program_.Call(program_.GetFunction(EqualsFnName),
                                      {value_, rhs.Get()}));
}

Bool String::operator!=(const String& rhs) const {
  return Bool(program_, program_.Call(program_.GetFunction(NotEqualsFnName),
                                      {value_, rhs.Get()}));
}

Bool String::operator<(const String& rhs) const {
  return Bool(program_, program_.Call(program_.GetFunction(LessThanFnName),
                                      {value_, rhs.Get()}));
}

Bool String::operator>(const String& rhs) const {
  return Bool(program_, program_.Call(program_.GetFunction(GreaterThanFnName),
                                      {value_, rhs.Get()}));
}

Bool String::operator<=(const String& rhs) const {
  return Bool(program_,
              program_.Call(program_.GetFunction(LessThanEqualsFnName),
                            {value_, rhs.Get()}));
}

Bool String::operator>=(const String& rhs) const {
  return Bool(program_,
              program_.Call(program_.GetFunction(GreaterThanEqualsFnName),
                            {value_, rhs.Get()}));
}

std::unique_ptr<String> String::ToPointer() const {
  return std::make_unique<String>(program_, value_);
}

void String::Print(Printer& printer) const { printer.Print(*this); }

Int64 String::Hash() const {
  return Int64(program_,
               program_.Call(program_.GetFunction(HashFnName), {value_}));
}

khir::ProgramBuilder& String::ProgramBuilder() const { return program_; }

khir::Value String::Get() const { return value_; }

khir::Value String::Constant(khir::ProgramBuilder& program,
                             std::string_view value) {
  auto char_ptr = program.GlobalConstCharArray(value);
  auto len = program.ConstI32(value.size());
  return program.ConstantStruct(program.GetStructType(StringStructName),
                                {char_ptr, len});
}

String String::Global(khir::ProgramBuilder& program, std::string_view value) {
  auto char_ptr = program.GlobalConstCharArray(value);
  auto len = program.ConstI32(value.size());
  auto str_struct = program.Global(
      program.GetStructType(StringStructName),
      program.ConstantStruct(program.GetStructType(StringStructName),
                             {char_ptr, len}));
  return String(program, str_struct);
}

void String::ForwardDeclare(khir::ProgramBuilder& program) {
  auto struct_type = program.StructType(
      {
          program.PointerType(program.I8Type()),
          program.I32Type(),
      },
      StringStructName);
  auto struct_ptr = program.PointerType(struct_type);

  program.DeclareExternalFunction(
      CopyFnName, program.VoidType(), {struct_ptr, struct_ptr},
      reinterpret_cast<void*>(runtime::String::Copy));
  program.DeclareExternalFunction(
      ContainsFnName, program.I1Type(), {struct_ptr, struct_ptr},
      reinterpret_cast<void*>(runtime::String::Contains));
  program.DeclareExternalFunction(
      LikeFnName, program.I1Type(),
      {struct_ptr, program.PointerType(program.I8Type())},
      reinterpret_cast<void*>(runtime::String::Like));
  program.DeclareExternalFunction(
      EndsWithFnName, program.I1Type(), {struct_ptr, struct_ptr},
      reinterpret_cast<void*>(runtime::String::EndsWith));
  program.DeclareExternalFunction(
      StartsWithFnName, program.I1Type(), {struct_ptr, struct_ptr},
      reinterpret_cast<void*>(runtime::String::StartsWith));
  program.DeclareExternalFunction(
      EqualsFnName, program.I1Type(), {struct_ptr, struct_ptr},
      reinterpret_cast<void*>(runtime::String::Equals));
  program.DeclareExternalFunction(
      NotEqualsFnName, program.I1Type(), {struct_ptr, struct_ptr},
      reinterpret_cast<void*>(runtime::String::NotEquals));
  program.DeclareExternalFunction(
      LessThanFnName, program.I1Type(), {struct_ptr, struct_ptr},
      reinterpret_cast<void*>(runtime::String::LessThan));
  program.DeclareExternalFunction(
      LessThanEqualsFnName, program.I1Type(), {struct_ptr, struct_ptr},
      reinterpret_cast<void*>(runtime::String::LessThanEquals));
  program.DeclareExternalFunction(
      GreaterThanFnName, program.I1Type(), {struct_ptr, struct_ptr},
      reinterpret_cast<void*>(runtime::String::GreaterThan));
  program.DeclareExternalFunction(
      GreaterThanEqualsFnName, program.I1Type(), {struct_ptr, struct_ptr},
      reinterpret_cast<void*>(runtime::String::GreaterThanEquals));
  program.DeclareExternalFunction(
      HashFnName, program.I64Type(), {struct_ptr},
      reinterpret_cast<void*>(runtime::String::Hash));
}

}  // namespace kush::compile::proxy