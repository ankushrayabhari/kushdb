#include "compile/proxy/printer.h"

#include "compile/ir_registry.h"
#include "compile/program_builder.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/float.h"
#include "compile/proxy/int.h"

namespace kush::compile::proxy {

constexpr std::string_view i8_fn_name("_ZN4kush4util5PrintEa");
constexpr std::string_view i16_fn_name("_ZN4kush4util5PrintEs");
constexpr std::string_view i32_fn_name("_ZN4kush4util5PrintEi");
constexpr std::string_view i64_fn_name("_ZN4kush4util5PrintEl");
constexpr std::string_view f64_fn_name("_ZN4kush4util5PrintEd");
constexpr std::string_view newline_fn_name("_ZN4kush4util12PrintNewlineEv");

template <typename T>
Printer<T>::Printer(ProgramBuilder<T>& program) : program_(program) {}

template <typename T>
void Printer<T>::Print(Int8<T>& t) {
  program_.Call(program_.GetFunction(i8_fn_name), {t.Get()});
}

template <typename T>
void Printer<T>::Print(Bool<T>& t) {
  program_.Call(program_.GetFunction(i8_fn_name), {t.Get()});
}

template <typename T>
void Printer<T>::Print(Int16<T>& t) {
  program_.Call(program_.GetFunction(i16_fn_name), {t.Get()});
}

template <typename T>
void Printer<T>::Print(Int32<T>& t) {
  program_.Call(program_.GetFunction(i32_fn_name), {t.Get()});
}

template <typename T>
void Printer<T>::Print(Int64<T>& t) {
  program_.Call(program_.GetFunction(i64_fn_name), {t.Get()});
}

template <typename T>
void Printer<T>::Print(Float64<T>& t) {
  program_.Call(program_.GetFunction(f64_fn_name), {t.Get()});
}

template <typename T>
void Printer<T>::PrintNewline() {
  program_.Call(program_.GetFunction(newline_fn_name), {});
}

template <typename T>
void Printer<T>::ForwardDeclare(ProgramBuilder<T>& program) {
  program.DeclareExternalFunction(i8_fn_name, program.VoidType(),
                                  {program.I8Type()});
  program.DeclareExternalFunction(i16_fn_name, program.VoidType(),
                                  {program.I16Type()});
  program.DeclareExternalFunction(i32_fn_name, program.VoidType(),
                                  {program.I32Type()});
  program.DeclareExternalFunction(i64_fn_name, program.VoidType(),
                                  {program.I64Type()});
  program.DeclareExternalFunction(f64_fn_name, program.VoidType(),
                                  {program.F64Type()});
  program.DeclareExternalFunction(newline_fn_name, program.VoidType(), {});
}

INSTANTIATE_ON_IR(Printer);

}  // namespace kush::compile::proxy