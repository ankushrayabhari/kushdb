#include "compile/proxy/printer.h"

#include "compile/khir/program_builder.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/float.h"
#include "compile/proxy/int.h"
#include "compile/proxy/string.h"

namespace kush::compile::proxy {

constexpr std::string_view i1_fn_name("_ZN4kush7runtime7Printer5PrintEb");
constexpr std::string_view i8_fn_name("_ZN4kush7runtime7Printer5PrintEa");
constexpr std::string_view i16_fn_name("_ZN4kush7runtime7Printer5PrintEs");
constexpr std::string_view i32_fn_name("_ZN4kush7runtime7Printer5PrintEi");
constexpr std::string_view i64_fn_name("_ZN4kush7runtime7Printer5PrintEl");
constexpr std::string_view f64_fn_name("_ZN4kush7runtime7Printer5PrintEd");
constexpr std::string_view newline_fn_name(
    "_ZN4kush7runtime7Printer12PrintNewlineEv");
constexpr std::string_view string_fn_name(
    "_ZN4kush7runtime7Printer11PrintStringEPNS0_6String6StringE");

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
  program.DeclareExternalFunction(i1_fn_name, program.VoidType(),
                                  {program.I1Type()});
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
  program.DeclareExternalFunction(
      string_fn_name, program.VoidType(),
      {program.PointerType(program.GetStructType(String::StringStructName))});
  program.DeclareExternalFunction(newline_fn_name, program.VoidType(), {});
}

}  // namespace kush::compile::proxy