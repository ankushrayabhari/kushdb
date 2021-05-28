#pragma once

#include "compile/khir/khir_program_builder.h"

namespace kush::compile::proxy {

// Forward declare the proxy types to avoid circular dependency
class Bool;
class Int8;
class Int16;
class Int32;
class Int64;
class Float64;
class String;

class Printer {
 public:
  Printer(khir::KHIRProgramBuilder& program);

  void Print(const Int8& t);
  void Print(const Bool& t);
  void Print(const Int16& t);
  void Print(const Int32& t);
  void Print(const Int64& t);
  void Print(const Float64& t);
  void Print(const String& t);
  void PrintNewline();

  static void ForwardDeclare(khir::KHIRProgramBuilder& program);

 private:
  khir::KHIRProgramBuilder& program_;
};

}  // namespace kush::compile::proxy