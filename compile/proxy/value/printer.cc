#include "runtime/printer.h"

#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

constexpr std::string_view i1_fn_name("kush::runtime::Printer::PrintBool");
constexpr std::string_view i8_fn_name("kush::runtime::Printer::PrintInt8");
constexpr std::string_view i16_fn_name("kush::runtime::Printer::PrintInt16");
constexpr std::string_view i32_fn_name("kush::runtime::Printer::PrintInt32");
constexpr std::string_view i64_fn_name("kush::runtime::Printer::PrintInt64");
constexpr std::string_view date_fn_name("kush::runtime::Printer::PrintDate");
constexpr std::string_view f64_fn_name("kush::runtime::Printer::PrintFloat64");
constexpr std::string_view newline_fn_name(
    "kush::runtime::Printer::PrintNewline");
constexpr std::string_view string_fn_name(
    "kush::runtime::Printer::PrintString");
constexpr std::string_view i1_fn_name_debug(
    "kush::runtime::Printer::PrintBoolDebug");
constexpr std::string_view i8_fn_name_debug(
    "kush::runtime::Printer::PrintInt8Debug");
constexpr std::string_view i16_fn_name_debug(
    "kush::runtime::Printer::PrintInt16Debug");
constexpr std::string_view i32_fn_name_debug(
    "kush::runtime::Printer::PrintInt32Debug");
constexpr std::string_view i64_fn_name_debug(
    "kush::runtime::Printer::PrintInt64Debug");
constexpr std::string_view date_fn_name_debug(
    "kush::runtime::Printer::PrintDateDebug");
constexpr std::string_view f64_fn_name_debug(
    "kush::runtime::Printer::PrintFloat64Debug");
constexpr std::string_view newline_fn_name_debug(
    "kush::runtime::Printer::PrintNewlineDebug");
constexpr std::string_view string_fn_name_debug(
    "kush::runtime::Printer::PrintStringDebug");

Printer::Printer(khir::ProgramBuilder& program, bool debug)
    : program_(program), debug_(debug) {}

void Printer::Print(const Int8& t) const {
  program_.Call(program_.GetFunction(!debug_ ? i8_fn_name : i8_fn_name_debug),
                {t.Get()});
}

void Printer::Print(const Bool& t) const {
  program_.Call(program_.GetFunction(!debug_ ? i1_fn_name : i1_fn_name_debug),
                {t.Get()});
}

void Printer::Print(const Int16& t) const {
  program_.Call(program_.GetFunction(!debug_ ? i16_fn_name : i16_fn_name_debug),
                {t.Get()});
}

void Printer::Print(const Int32& t) const {
  program_.Call(program_.GetFunction(!debug_ ? i32_fn_name : i32_fn_name_debug),
                {t.Get()});
}

void Printer::Print(const Int64& t) const {
  program_.Call(program_.GetFunction(!debug_ ? i64_fn_name : i64_fn_name_debug),
                {t.Get()});
}

void Printer::Print(const Float64& t) const {
  program_.Call(program_.GetFunction(!debug_ ? f64_fn_name : f64_fn_name_debug),
                {t.Get()});
}

void Printer::Print(const String& t) const {
  program_.Call(
      program_.GetFunction(!debug_ ? string_fn_name : string_fn_name_debug),
      {t.Get()});
}

void Printer::Print(const Date& t) const {
  program_.Call(
      program_.GetFunction(!debug_ ? date_fn_name : date_fn_name_debug),
      {t.Get()});
}

void Printer::PrintNewline() const {
  program_.Call(
      program_.GetFunction(!debug_ ? newline_fn_name : newline_fn_name_debug),
      {});
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
      date_fn_name, program.VoidType(), {program.I64Type()},
      reinterpret_cast<void*>(&runtime::Printer::PrintDate));
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

  program.DeclareExternalFunction(
      i1_fn_name_debug, program.VoidType(), {program.I1Type()},
      reinterpret_cast<void*>(&runtime::Printer::PrintBoolDebug));
  program.DeclareExternalFunction(
      i8_fn_name_debug, program.VoidType(), {program.I8Type()},
      reinterpret_cast<void*>(&runtime::Printer::PrintInt8Debug));
  program.DeclareExternalFunction(
      i16_fn_name_debug, program.VoidType(), {program.I16Type()},
      reinterpret_cast<void*>(&runtime::Printer::PrintInt16Debug));
  program.DeclareExternalFunction(
      i32_fn_name_debug, program.VoidType(), {program.I32Type()},
      reinterpret_cast<void*>(&runtime::Printer::PrintInt32Debug));
  program.DeclareExternalFunction(
      i64_fn_name_debug, program.VoidType(), {program.I64Type()},
      reinterpret_cast<void*>(&runtime::Printer::PrintInt64Debug));
  program.DeclareExternalFunction(
      date_fn_name_debug, program.VoidType(), {program.I64Type()},
      reinterpret_cast<void*>(&runtime::Printer::PrintDateDebug));
  program.DeclareExternalFunction(
      f64_fn_name_debug, program.VoidType(), {program.F64Type()},
      reinterpret_cast<void*>(&runtime::Printer::PrintFloat64Debug));
  program.DeclareExternalFunction(
      string_fn_name_debug, program.VoidType(),
      {program.PointerType(program.GetStructType(String::StringStructName))},
      reinterpret_cast<void*>(&runtime::Printer::PrintStringDebug));
  program.DeclareExternalFunction(
      newline_fn_name_debug, program.VoidType(), {},
      reinterpret_cast<void*>(&runtime::Printer::PrintNewlineDebug));
}

}  // namespace kush::compile::proxy