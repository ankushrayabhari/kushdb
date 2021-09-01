#include "compile/proxy/printer.h"

#include "compile/proxy/bool.h"
#include "compile/proxy/float.h"
#include "compile/proxy/int.h"
#include "compile/proxy/string.h"
#include "khir/program_builder.h"
#include "runtime/printer.h"

namespace kush::compile::proxy {

constexpr std::string_view i1_fn_name("kush::runtime::Printer::PrintBool");
constexpr std::string_view i8_fn_name("kush::runtime::Printer::PrintInt8");
constexpr std::string_view i16_fn_name("kush::runtime::Printer::PrintInt16");
constexpr std::string_view i32_fn_name("kush::runtime::Printer::PrintInt32");
constexpr std::string_view i64_fn_name("kush::runtime::Printer::PrintInt64");
constexpr std::string_view f64_fn_name("kush::runtime::Printer::PrintFloat64");
constexpr std::string_view newline_fn_name(
    "kush::runtime::Printer::PrintNewline");
constexpr std::string_view string_fn_name(
    "kush::runtime::Printer::PrintString");

Printer::Printer(khir::ProgramBuilder& program) : program_(program) {}

void Printer::Print(const Int8& t) {
  program_.Call(program_.GetFunction(i8_fn_name), {t.Get()});
}

void Printer::Print(const Bool& t) {
  program_.Call(program_.GetFunction(i1_fn_name), {t.Get()});
}

void Printer::Print(const Int16& t) {
  program_.Call(program_.GetFunction(i16_fn_name), {t.Get()});
}

void Printer::Print(const Int32& t) {
  program_.Call(program_.GetFunction(i32_fn_name), {t.Get()});
}

void Printer::Print(const Int64& t) {
  program_.Call(program_.GetFunction(i64_fn_name), {t.Get()});
}

void Printer::Print(const Float64& t) {
  program_.Call(program_.GetFunction(f64_fn_name), {t.Get()});
}

void Printer::Print(const String& t) {
  program_.Call(program_.GetFunction(string_fn_name), {t.Get()});
}

void Printer::PrintNewline() {
  program_.Call(program_.GetFunction(newline_fn_name), {});
}

void Printer::ForwardDeclare(khir::ProgramBuilder& program) {
  program.DeclareExternalFunction(
      i1_fn_name, program.VoidType(), {program.I1Type()},
      reinterpret_cast<void*>(&runtime::Printer::PrintBool));
  program.DeclareExternalFunction(
      i8_fn_name, program.VoidType(), {program.I8Type()},
      reinterpret_cast<void*>(&runtime::Printer::PrintInt8));
  program.DeclareExternalFunction(
      i16_fn_name, program.VoidType(), {program.I16Type()},
      reinterpret_cast<void*>(&runtime::Printer::PrintInt16));
  program.DeclareExternalFunction(
      i32_fn_name, program.VoidType(), {program.I32Type()},
      reinterpret_cast<void*>(&runtime::Printer::PrintInt32));
  program.DeclareExternalFunction(
      i64_fn_name, program.VoidType(), {program.I64Type()},
      reinterpret_cast<void*>(&runtime::Printer::PrintInt64));
  program.DeclareExternalFunction(
      f64_fn_name, program.VoidType(), {program.F64Type()},
      reinterpret_cast<void*>(&runtime::Printer::PrintFloat64));
  program.DeclareExternalFunction(
      string_fn_name, program.VoidType(),
      {program.PointerType(program.GetStructType(String::StringStructName))},
      reinterpret_cast<void*>(&runtime::Printer::PrintString));
  program.DeclareExternalFunction(
      newline_fn_name, program.VoidType(), {},
      reinterpret_cast<void*>(&runtime::Printer::PrintNewline));
}

}  // namespace kush::compile::proxy