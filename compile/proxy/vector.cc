#include "compile/proxy/vector.h"

#include "compile/proxy/struct.h"
#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"
#include "runtime/vector.h"

namespace kush::compile::proxy {

constexpr std::string_view CreateFnName("kush::runtime::Vector::Create");

constexpr std::string_view PushBackFnName("kush::runtime::Vector::PushBack");

constexpr std::string_view GetFnName("kush::runtime::Vector::Get");

constexpr std::string_view SizeFnName("kush::runtime::Vector::Size");

constexpr std::string_view FreeFnName("kush::runtime::Vector::Free");

constexpr std::string_view SortFnName("kush::runtime::Vector::Sort");

Vector::Vector(khir::ProgramBuilder& program, StructBuilder& content,
               bool global)
    : program_(program),
      content_(content),
      content_type_(content_.Type()),
      value_(global ? program_.Global(
                          false, true,
                          program_.GetStructType(Vector::VectorStructName),
                          program_.ConstantStruct(
                              program_.GetStructType(Vector::VectorStructName),
                              {
                                  program.ConstI64(0),
                                  program.ConstI32(0),
                                  program.ConstI32(0),
                                  program.NullPtr(
                                      program.PointerType(program.I8Type())),
                              }))
                    : program.Alloca(program.GetStructType(VectorStructName))) {
  auto element_size = program_.SizeOf(content_type_);
  auto initial_capacity = program_.ConstI32(2);
  program_.Call(program_.GetFunction(CreateFnName),
                {value_, element_size, initial_capacity});
}

Vector::Vector(khir::ProgramBuilder& program, StructBuilder& content,
               khir::Value v)
    : program_(program),
      content_(content),
      content_type_(content_.Type()),
      value_(v) {}

void Vector::Reset() {
  program_.Call(program_.GetFunction(FreeFnName), {value_});
}

khir::Value Vector::Get() const { return value_; }

Struct Vector::operator[](const Int32& idx) {
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
      {struct_ptr, program.I64Type(), program.I32Type()},
      reinterpret_cast<void*>(&kush::runtime::Vector::Create));

  program.DeclareExternalFunction(
      PushBackFnName, program.PointerType(program.I8Type()), {struct_ptr},
      reinterpret_cast<void*>(&kush::runtime::Vector::PushBack));

  program.DeclareExternalFunction(
      GetFnName, program.PointerType(program.I8Type()),
      {struct_ptr, program.I32Type()},
      reinterpret_cast<void*>(&kush::runtime::Vector::Get));

  program.DeclareExternalFunction(
      SizeFnName, program.I32Type(), {struct_ptr},
      reinterpret_cast<void*>(&kush::runtime::Vector::Size));

  program.DeclareExternalFunction(
      FreeFnName, program.VoidType(), {struct_ptr},
      reinterpret_cast<void*>(&kush::runtime::Vector::Free));

  program.DeclareExternalFunction(
      SortFnName, program.VoidType(),
      {struct_ptr,
       program.PointerType(program.FunctionType(
           program.I1Type(), {program.PointerType(program.I8Type()),
                              program.PointerType(program.I8Type())}))},
      reinterpret_cast<void*>(&kush::runtime::Vector::Sort));
}

}  // namespace kush::compile::proxy