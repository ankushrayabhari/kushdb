#include "compile/proxy/string.h"

#include "compile/khir/program_builder.h"
#include "compile/proxy/bool.h"

namespace kush::compile::proxy {

constexpr std::string_view CopyFnName("_ZN4kush4data4CopyEPNS0_6StringES2_");
constexpr std::string_view FreeFnName("_ZN4kush4data4FreeEPNS0_6StringE");
constexpr std::string_view ContainsFnName(
    "_ZN4kush4data8ContainsEPNS0_6StringES2_");
constexpr std::string_view EndsWithFnName(
    "_ZN4kush4data8EndsWithEPNS0_6StringES2_");
constexpr std::string_view StartsWithFnName(
    "_ZN4kush4data10StartsWithEPNS0_6StringES2_");
constexpr std::string_view EqualsFnName(
    "_ZN4kush4data6EqualsEPNS0_6StringES2_");
constexpr std::string_view NotEqualsFnName(
    "_ZN4kush4data9NotEqualsEPNS0_6StringES2_");
constexpr std::string_view LessThanFnName(
    "_ZN4kush4data8LessThanEPNS0_6StringES2_");
constexpr std::string_view HashFnName("_ZN4kush4data4HashEPNS0_6StringE");

const std::string_view String::StringStructName("kush::data::String");

String::String(khir::ProgramBuilder& program, const khir::Value& value)
    : program_(program), value_(value) {}

void String::Copy(const String& rhs) {
  program_.Call(program_.GetFunction(CopyFnName), {value_, rhs.Get()});
}

void String::Reset() {
  program_.Call(program_.GetFunction(FreeFnName), {value_});
}

Bool String::Contains(const String& rhs) {
  return Bool(program_, program_.Call(program_.GetFunction(ContainsFnName),
                                      {value_, rhs.Get()}));
}

Bool String::StartsWith(const String& rhs) {
  return Bool(program_, program_.Call(program_.GetFunction(StartsWithFnName),
                                      {value_, rhs.Get()}));
}

Bool String::EndsWith(const String& rhs) {
  return Bool(program_, program_.Call(program_.GetFunction(EndsWithFnName),
                                      {value_, rhs.Get()}));
}

Bool String::operator==(const String& rhs) {
  return Bool(program_, program_.Call(program_.GetFunction(EqualsFnName),
                                      {value_, rhs.Get()}));
}

Bool String::operator!=(const String& rhs) {
  return Bool(program_, program_.Call(program_.GetFunction(NotEqualsFnName),
                                      {value_, rhs.Get()}));
}

Bool String::operator<(const String& rhs) {
  return Bool(program_, program_.Call(program_.GetFunction(LessThanFnName),
                                      {value_, rhs.Get()}));
}

std::unique_ptr<String> String::ToPointer() {
  return std::make_unique<String>(program_, value_);
}

std::unique_ptr<Value> String::EvaluateBinary(
    plan::BinaryArithmeticOperatorType op_type, Value& rhs_generic) {
  String& rhs = dynamic_cast<String&>(rhs_generic);
  switch (op_type) {
    case plan::BinaryArithmeticOperatorType::STARTS_WITH:
      return StartsWith(rhs).ToPointer();

    case plan::BinaryArithmeticOperatorType::ENDS_WITH:
      return EndsWith(rhs).ToPointer();

    case plan::BinaryArithmeticOperatorType::CONTAINS:
      return Contains(rhs).ToPointer();

    case plan::BinaryArithmeticOperatorType::EQ:
      return (*this == rhs).ToPointer();

    case plan::BinaryArithmeticOperatorType::NEQ:
      return (*this != rhs).ToPointer();

    case plan::BinaryArithmeticOperatorType::LT:
      return (*this < rhs).ToPointer();

    default:
      throw std::runtime_error("Invalid operator on string");
  }
}

void String::Print(proxy::Printer& printer) { printer.Print(*this); }

khir::Value String::Hash() {
  return program_.Call(program_.GetFunction(HashFnName), {value_});
}

khir::Value String::Get() const { return value_; }

String String::Constant(khir::ProgramBuilder& program, std::string_view value) {
  auto char_ptr = program.ConstCharArray(value);
  auto len = program.ConstI32(value.size());
  auto str_struct = program.Global(
      true, false, program.GetStructType(StringStructName),
      program.ConstantStruct(program.GetStructType(StringStructName),
                             {char_ptr(), len})());
  return String(program, str_struct());
}

void String::ForwardDeclare(khir::ProgramBuilder& program) {
  auto struct_type = program.StructType(
      {
          program.PointerType(program.I8Type()),
          program.I32Type(),
      },
      StringStructName);
  auto struct_ptr = program.PointerType(struct_type);

  program.DeclareExternalFunction(CopyFnName, program.VoidType(),
                                  {struct_ptr, struct_ptr});
  program.DeclareExternalFunction(FreeFnName, program.VoidType(), {struct_ptr});
  program.DeclareExternalFunction(ContainsFnName, program.I1Type(),
                                  {struct_ptr, struct_ptr});
  program.DeclareExternalFunction(EndsWithFnName, program.I1Type(),
                                  {struct_ptr, struct_ptr});
  program.DeclareExternalFunction(StartsWithFnName, program.I1Type(),
                                  {struct_ptr, struct_ptr});
  program.DeclareExternalFunction(EqualsFnName, program.I1Type(),
                                  {struct_ptr, struct_ptr});
  program.DeclareExternalFunction(NotEqualsFnName, program.I1Type(),
                                  {struct_ptr, struct_ptr});
  program.DeclareExternalFunction(LessThanFnName, program.I1Type(),
                                  {struct_ptr, struct_ptr});
  program.DeclareExternalFunction(HashFnName, program.I64Type(), {struct_ptr});
}

}  // namespace kush::compile::proxy