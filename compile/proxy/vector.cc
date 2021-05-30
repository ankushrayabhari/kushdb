#include "compile/proxy/vector.h"

#include "compile/khir/program_builder.h"
#include "compile/proxy/if.h"
#include "compile/proxy/int.h"
#include "compile/proxy/ptr.h"
#include "compile/proxy/struct.h"

namespace kush::compile::proxy {

const std::string_view CreateFnName("_ZN4kush4data6CreateEPNS0_6VectorEli");

const std::string_view PushBackFnName("_ZN4kush4data8PushBackEPNS0_6VectorE");

const std::string_view GetFnName("_ZN4kush4data3GetEPNS0_6VectorEi");

const std::string_view SizeFnName("_ZN4kush4data4SizeEPNS0_6VectorE");

const std::string_view FreeFnName("_ZN4kush4data4FreeEPNS0_6VectorE");

const std::string_view SortFnName("_ZN4kush4data4SortEPNS0_6VectorEPFbPaS3_E");

const std::string_view Vector::VectorStructName("kush::data::Vector");

Vector::Vector(khir::ProgramBuilder& program, StructBuilder& content,
               bool global)
    : program_(program),
      content_(content),
      content_type_(content_.Type()),
      value_(global
                 ? program_.Global(
                       false, true, program_.GetStructType(VectorStructName),
                       program_.ConstantStruct(
                           program_.GetStructType(VectorStructName),
                           {
                               program.ConstI64(0),
                               program.ConstI32(0),
                               program.ConstI32(0),
                               program.NullPtr(
                                   program.PointerType(program.I8Type())),
                           })())()
                 : program_.Alloca(program_.GetStructType(VectorStructName))) {
  auto element_size = program_.SizeOf(content_type_);
  auto initial_capacity = program_.ConstI32(2);
  program_.Call(program_.GetFunction(CreateFnName),
                {value_, element_size, initial_capacity});
}

Vector::Vector(khir::ProgramBuilder& program, StructBuilder& content,
               const khir::Value& value)
    : program_(program),
      content_(content),
      content_type_(content_.Type()),
      value_(value) {}

void Vector::Reset() {
  program_.Call(program_.GetFunction(FreeFnName), {value_});
}

Struct Vector::operator[](const proxy::Int32& idx) {
  auto ptr =
      program_.Call(program_.GetFunction(GetFnName), {value_, idx.Get()});
  auto ptr_type = program_.PointerType(content_type_);
  return Struct(program_, content_, program_.PointerCast(ptr, ptr_type));
}

Struct Vector::PushBack() {
  auto ptr = program_.Call(program_.GetFunction(PushBackFnName), {value_});
  auto ptr_type = program_.PointerType(content_type_);
  return Struct(program_, content_, program_.PointerCast(ptr, ptr_type));
}

void Vector::Sort(const khir::FunctionRef& comp) {
  program_.Call(program_.GetFunction(SortFnName),
                {value_, program_.GetFunctionPointer(comp)});
}

Int32 Vector::Size() {
  return Int32(program_,
               program_.Call(program_.GetFunction(SizeFnName), {value_}));
}

void Vector::ForwardDeclare(khir::ProgramBuilder& program) {
  auto struct_type = program.StructType(
      {
          program.I64Type(),
          program.I32Type(),
          program.I32Type(),
          program.PointerType(program.I8Type()),
      },
      VectorStructName);
  auto struct_ptr = program.PointerType(struct_type);

  program.DeclareExternalFunction(
      CreateFnName, program.VoidType(),
      {struct_ptr, program.I64Type(), program.I32Type()});

  program.DeclareExternalFunction(
      PushBackFnName, program.PointerType(program.I8Type()), {struct_ptr});

  program.DeclareExternalFunction(GetFnName,
                                  program.PointerType(program.I8Type()),
                                  {struct_ptr, program.I32Type()});

  program.DeclareExternalFunction(SizeFnName, program.I32Type(), {struct_ptr});

  program.DeclareExternalFunction(FreeFnName, program.VoidType(), {struct_ptr});

  program.DeclareExternalFunction(
      SortFnName, program.VoidType(),
      {struct_ptr,
       program.PointerType(program.FunctionType(
           program.I1Type(), {program.PointerType(program.I8Type()),
                              program.PointerType(program.I8Type())}))});
}

}  // namespace kush::compile::proxy