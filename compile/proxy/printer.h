#pragma once

#include "compile/program_builder.h"

namespace kush::compile::proxy {

// Forward declare the proxy types to avoid circular dependency
template <typename T>
class Bool;

template <typename T>
class Int8;

template <typename T>
class Int16;

template <typename T>
class Int32;

template <typename T>
class Int64;

template <typename T>
class Float64;

// Wrapper object for forward declared functions in generated program.
template <typename T>
class ForwardDeclaredPrintFunctions {
 public:
  ForwardDeclaredPrintFunctions(
      typename ProgramBuilder<T>::Function& print_int8,
      typename ProgramBuilder<T>::Function& print_int16,
      typename ProgramBuilder<T>::Function& print_int32,
      typename ProgramBuilder<T>::Function& print_int64,
      typename ProgramBuilder<T>::Function& print_float64,
      typename ProgramBuilder<T>::Function& print_newline);

  typename ProgramBuilder<T>::Function& PrintInt8();
  typename ProgramBuilder<T>::Function& PrintInt16();
  typename ProgramBuilder<T>::Function& PrintInt32();
  typename ProgramBuilder<T>::Function& PrintInt64();
  typename ProgramBuilder<T>::Function& PrintFloat64();
  typename ProgramBuilder<T>::Function& PrintNewline();

 private:
  typename ProgramBuilder<T>::Function& print_int8_;
  typename ProgramBuilder<T>::Function& print_int16_;
  typename ProgramBuilder<T>::Function& print_int32_;
  typename ProgramBuilder<T>::Function& print_int64_;
  typename ProgramBuilder<T>::Function& print_float64_;
  typename ProgramBuilder<T>::Function& print_newline_;
};

// Generate print calls in generated program.
template <typename T>
class Printer {
 public:
  Printer(ProgramBuilder<T>& program, ForwardDeclaredPrintFunctions<T>& funcs);

  void Print(Int8<T>& t);
  void Print(Bool<T>& t);
  void Print(Int16<T>& t);
  void Print(Int32<T>& t);
  void Print(Int64<T>& t);
  void Print(Float64<T>& t);
  void PrintNewline();

  static ForwardDeclaredPrintFunctions<T> ForwardDeclare(
      ProgramBuilder<T>& program);

 private:
  ProgramBuilder<T>& program_;
  ForwardDeclaredPrintFunctions<T>& funcs_;
};

}  // namespace kush::compile::proxy