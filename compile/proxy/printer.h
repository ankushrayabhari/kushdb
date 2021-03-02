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

// Generate print calls in generated program.
template <typename T>
class Printer {
 public:
  Printer(ProgramBuilder<T>& program);

  void Print(Int8<T>& t);
  void Print(Bool<T>& t);
  void Print(Int16<T>& t);
  void Print(Int32<T>& t);
  void Print(Int64<T>& t);
  void Print(Float64<T>& t);
  void PrintNewline();

  static void ForwardDeclare(ProgramBuilder<T>& program);

 private:
  ProgramBuilder<T>& program_;
};

}  // namespace kush::compile::proxy