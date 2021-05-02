#include "compile/proxy/vector.h"

#include "compile/ir_registry.h"
#include "compile/program_builder.h"
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

template <typename T>
Vector<T>::Vector(ProgramBuilder<T>& program, StructBuilder<T>& content,
                  bool global)
    : program_(program),
      content_(content),
      content_type_(content_.Type()),
      value_(
          global
              ? program_.GlobalStruct(
                    false, program_.GetStructType(VectorStructName),
                    {
                        program.ConstI64(0),
                        program.ConstI32(0),
                        program.ConstI32(0),
                        program.NullPtr(program.PointerType(program.I8Type())),
                    })
              : program_.Alloca(program_.GetStructType(VectorStructName))) {
  auto element_size = program_.SizeOf(content_type_);
  auto initial_capacity = program_.ConstI32(2);
  program_.Call(program_.GetFunction(CreateFnName),
                {value_, element_size, initial_capacity});
}

template <typename T>
Vector<T>::Vector(ProgramBuilder<T>& program, StructBuilder<T>& content,
                  const typename ProgramBuilder<T>::Value& value)
    : program_(program),
      content_(content),
      content_type_(content_.Type()),
      value_(value) {}

template <typename T>
void Vector<T>::Reset() {
  program_.Call(program_.GetFunction(FreeFnName), {value_});
}

template <typename T>
Struct<T> Vector<T>::operator[](const proxy::Int32<T>& idx) {
  auto ptr =
      program_.Call(program_.GetFunction(GetFnName), {value_, idx.Get()});
  auto ptr_type = program_.PointerType(content_type_);
  return Struct<T>(program_, content_, program_.PointerCast(ptr, ptr_type));
}

template <typename T>
Struct<T> Vector<T>::PushBack() {
  auto ptr = program_.Call(program_.GetFunction(PushBackFnName), {value_});
  auto ptr_type = program_.PointerType(content_type_);
  return Struct<T>(program_, content_, program_.PointerCast(ptr, ptr_type));
}

template <typename T>
void Vector<T>::Sort(const typename ProgramBuilder<T>::Function& comp) {
  program_.Call(program_.GetFunction(SortFnName), {value_, comp});
}

template <typename T>
Int32<T> Vector<T>::Size() {
  return Int32<T>(program_,
                  program_.Call(program_.GetFunction(SizeFnName), {value_}));
}

template <typename T>
void Vector<T>::ForwardDeclare(ProgramBuilder<T>& program) {
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

INSTANTIATE_ON_IR(Vector);

}  // namespace kush::compile::proxy