#include "compile/proxy/printer.h"

#include "compile/ir_registry.h"
#include "compile/program_builder.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/float.h"
#include "compile/proxy/int.h"

namespace kush::compile::proxy {

template <typename T>
ForwardDeclaredPrintFunctions<T>::ForwardDeclaredPrintFunctions(
    typename ProgramBuilder<T>::Function& print_int8,
    typename ProgramBuilder<T>::Function& print_int16,
    typename ProgramBuilder<T>::Function& print_int32,
    typename ProgramBuilder<T>::Function& print_int64,
    typename ProgramBuilder<T>::Function& print_float64,
    typename ProgramBuilder<T>::Function& print_newline)
    : print_int8_(print_int8),
      print_int16_(print_int16),
      print_int32_(print_int32),
      print_int64_(print_int64),
      print_float64_(print_float64),
      print_newline_(print_newline) {}

template <typename T>
typename ProgramBuilder<T>::Function&
ForwardDeclaredPrintFunctions<T>::PrintInt8() {
  return print_int8_;
}

template <typename T>
typename ProgramBuilder<T>::Function&
ForwardDeclaredPrintFunctions<T>::PrintInt16() {
  return print_int16_;
}

template <typename T>
typename ProgramBuilder<T>::Function&
ForwardDeclaredPrintFunctions<T>::PrintInt32() {
  return print_int32_;
}

template <typename T>
typename ProgramBuilder<T>::Function&
ForwardDeclaredPrintFunctions<T>::PrintInt64() {
  return print_int64_;
}

template <typename T>
typename ProgramBuilder<T>::Function&
ForwardDeclaredPrintFunctions<T>::PrintFloat64() {
  return print_float64_;
}

template <typename T>
typename ProgramBuilder<T>::Function&
ForwardDeclaredPrintFunctions<T>::PrintNewline() {
  return print_newline_;
}

INSTANTIATE_ON_IR(ForwardDeclaredPrintFunctions);

template <typename T>
Printer<T>::Printer(ProgramBuilder<T>& program,
                    ForwardDeclaredPrintFunctions<T>& funcs)
    : program_(program), funcs_(funcs) {}

template <typename T>
void Printer<T>::Print(Int8<T>& t) {
  program_.Call(funcs_.PrintInt8(), {t.Get()});
}

template <typename T>
void Printer<T>::Print(Bool<T>& t) {
  program_.Call(funcs_.PrintInt8(), {t.Get()});
}

template <typename T>
void Printer<T>::Print(Int16<T>& t) {
  program_.Call(funcs_.PrintInt16(), {t.Get()});
}

template <typename T>
void Printer<T>::Print(Int32<T>& t) {
  program_.Call(funcs_.PrintInt32(), {t.Get()});
}

template <typename T>
void Printer<T>::Print(Int64<T>& t) {
  program_.Call(funcs_.PrintInt64(), {t.Get()});
}

template <typename T>
void Printer<T>::Print(Float64<T>& t) {
  program_.Call(funcs_.PrintFloat64(), {t.Get()});
}

template <typename T>
void Printer<T>::PrintNewline() {
  program_.Call(funcs_.PrintNewline(), {});
}

template <typename T>
ForwardDeclaredPrintFunctions<T> Printer<T>::ForwardDeclare(
    ProgramBuilder<T>& program) {
  auto& i8_fn = program.DeclareExternalFunction(
      "_ZN4kush4util5PrintEa", program.VoidType(), {program.I8Type()});
  auto& i16_fn = program.DeclareExternalFunction(
      "_ZN4kush4util5PrintEs", program.VoidType(), {program.I16Type()});
  auto& i32_fn = program.DeclareExternalFunction(
      "_ZN4kush4util5PrintEi", program.VoidType(), {program.I32Type()});
  auto& i64_fn = program.DeclareExternalFunction(
      "_ZN4kush4util5PrintEl", program.VoidType(), {program.I64Type()});
  auto& f64_fn = program.DeclareExternalFunction(
      "_ZN4kush4util5PrintEd", program.VoidType(), {program.F64Type()});
  auto& newline_fn = program.DeclareExternalFunction(
      "_ZN4kush4util12PrintNewlineEv", program.VoidType(), {});

  return ForwardDeclaredPrintFunctions<T>(i8_fn, i16_fn, i32_fn, i64_fn, f64_fn,
                                          newline_fn);
}

INSTANTIATE_ON_IR(Printer);

}  // namespace kush::compile::proxy