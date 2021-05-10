#include "compile/proxy/string.h"

#include "compile/ir_registry.h"
#include "compile/program_builder.h"
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

template <typename T>
String<T>::String(ProgramBuilder<T>& program,
                  const typename ProgramBuilder<T>::Value& value)
    : program_(program), value_(value) {}

template <typename T>
void String<T>::Copy(const String<T>& rhs) {
  program_.Call(program_.GetFunction(CopyFnName), {value_, rhs.Get()});
}

template <typename T>
void String<T>::Reset() {
  program_.Call(program_.GetFunction(FreeFnName), {value_});
}

template <typename T>
Bool<T> String<T>::Contains(const String<T>& rhs) {
  return Bool<T>(program_, program_.Call(program_.GetFunction(ContainsFnName),
                                         {value_, rhs.Get()}));
}

template <typename T>
Bool<T> String<T>::StartsWith(const String<T>& rhs) {
  return Bool<T>(program_, program_.Call(program_.GetFunction(StartsWithFnName),
                                         {value_, rhs.Get()}));
}

template <typename T>
Bool<T> String<T>::EndsWith(const String<T>& rhs) {
  return Bool<T>(program_, program_.Call(program_.GetFunction(EndsWithFnName),
                                         {value_, rhs.Get()}));
}

template <typename T>
Bool<T> String<T>::operator==(const String<T>& rhs) {
  return Bool<T>(program_, program_.Call(program_.GetFunction(EqualsFnName),
                                         {value_, rhs.Get()}));
}

template <typename T>
Bool<T> String<T>::operator!=(const String<T>& rhs) {
  return Bool<T>(program_, program_.Call(program_.GetFunction(NotEqualsFnName),
                                         {value_, rhs.Get()}));
}

template <typename T>
Bool<T> String<T>::operator<(const String<T>& rhs) {
  return Bool<T>(program_, program_.Call(program_.GetFunction(LessThanFnName),
                                         {value_, rhs.Get()}));
}

template <typename T>
std::unique_ptr<String<T>> String<T>::ToPointer() {
  return std::make_unique<String<T>>(program_, value_);
}

template <typename T>
std::unique_ptr<Value<T>> String<T>::EvaluateBinary(
    plan::BinaryArithmeticOperatorType op_type, Value<T>& rhs_generic) {
  String<T>& rhs = dynamic_cast<String<T>&>(rhs_generic);
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

template <typename T>
void String<T>::Print(proxy::Printer<T>& printer) {
  printer.Print(*this);
}

template <typename T>
typename ProgramBuilder<T>::Value String<T>::Hash() {
  return program_.Call(program_.GetFunction(HashFnName), {value_});
}

template <typename T>
typename ProgramBuilder<T>::Value String<T>::Get() const {
  return value_;
}

template <typename T>
String<T> String<T>::Constant(ProgramBuilder<T>& program,
                              std::string_view value) {
  auto str_ptr = program.GlobalConstString(value);
  auto str = program.GetElementPtr(
      program.ArrayType(program.I8Type(), value.size() + 1), str_ptr, {0, 0});
  auto len = program.ConstI32(value.size());
  return String<T>(
      program, program.GlobalStruct(
                   true, program.GetStructType(StringStructName), {str, len}));
}

template <typename T>
void String<T>::ForwardDeclare(ProgramBuilder<T>& program) {
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

INSTANTIATE_ON_IR(String);

}  // namespace kush::compile::proxy