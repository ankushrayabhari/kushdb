#include "compile/proxy/vector.h"

#include "compile/ir_registry.h"
#include "compile/program_builder.h"
#include "compile/proxy/if.h"
#include "compile/proxy/int.h"
#include "compile/proxy/ptr.h"
#include "compile/proxy/struct.h"

namespace kush::compile::proxy {

template <typename T>
ForwardDeclaredVectorFunctions<T>::ForwardDeclaredVectorFunctions(
    typename ProgramBuilder<T>::Type& vector_type,
    typename ProgramBuilder<T>::Function& create_func,
    typename ProgramBuilder<T>::Function& push_back_func,
    typename ProgramBuilder<T>::Function& get_func,
    typename ProgramBuilder<T>::Function& size_func,
    typename ProgramBuilder<T>::Function& free_func)
    : vector_type_(vector_type),
      create_func_(create_func),
      push_back_func_(push_back_func),
      get_func_(get_func),
      size_func_(size_func),
      free_func_(free_func) {}

template <typename T>
typename ProgramBuilder<T>::Type&
ForwardDeclaredVectorFunctions<T>::VectorType() {
  return vector_type_;
}

template <typename T>
typename ProgramBuilder<T>::Function&
ForwardDeclaredVectorFunctions<T>::Create() {
  return create_func_;
}

template <typename T>
typename ProgramBuilder<T>::Function&
ForwardDeclaredVectorFunctions<T>::PushBack() {
  return push_back_func_;
}

template <typename T>
typename ProgramBuilder<T>::Function& ForwardDeclaredVectorFunctions<T>::Get() {
  return get_func_;
}

template <typename T>
typename ProgramBuilder<T>::Function&
ForwardDeclaredVectorFunctions<T>::Size() {
  return size_func_;
}

template <typename T>
typename ProgramBuilder<T>::Function&
ForwardDeclaredVectorFunctions<T>::Free() {
  return free_func_;
}

INSTANTIATE_ON_IR(ForwardDeclaredVectorFunctions);

template <typename T>
Vector<T>::Vector(ProgramBuilder<T>& program,
                  ForwardDeclaredVectorFunctions<T>& vector_funcs,
                  StructBuilder<T>& content)
    : program_(program),
      vector_funcs_(vector_funcs),
      content_(content),
      content_type_(content_.Type()),
      value_(program_.Alloca(vector_funcs_.VectorType())) {
  auto& element_size = program_.SizeOf(content_type_);
  auto& initial_capacity = program_.ConstUI32(2);
  program_.Call(vector_funcs_.Create(),
                {value_, element_size, initial_capacity});
}

template <typename T>
Vector<T>::~Vector() {
  program_.Call(vector_funcs_.Free(), {value_});
}

template <typename T>
Struct<T> Vector<T>::operator[](const proxy::UInt32<T>& idx) {
  auto& ptr = program_.Call(vector_funcs_.Get(), {value_, idx.Get()});
  auto& ptr_type = program_.PointerType(content_type_);
  return Struct<T>(program_, content_, program_.PointerCast(ptr, ptr_type));
}

template <typename T>
Struct<T> Vector<T>::PushBack() {
  auto& ptr = program_.Call(vector_funcs_.PushBack(), {value_});
  auto& ptr_type = program_.PointerType(content_type_);
  return Struct<T>(program_, content_, program_.PointerCast(ptr, ptr_type));
}

template <typename T>
void Vector<T>::Sort(typename ProgramBuilder<T>::Function& comp) {
  // TODO
}

template <typename T>
UInt32<T> Vector<T>::Size() {
  return UInt32<T>(program_, program_.Call(vector_funcs_.Size(), {value_}));
}

template <typename T>
ForwardDeclaredVectorFunctions<T> Vector<T>::ForwardDeclare(
    ProgramBuilder<T>& program) {
  auto& struct_type = program.StructType({
      program.I64Type(),
      program.I32Type(),
      program.I32Type(),
      program.PointerType(program.I8Type()),
  });
  auto& struct_ptr = program.PointerType(struct_type);

  auto& create_fn = program.DeclareExternalFunction(
      "_ZN4kush4data6CreateEPNS0_6VectorEmj", program.VoidType(),
      {struct_ptr, program.I64Type(), program.I32Type()});

  auto& push_back_fn = program.DeclareExternalFunction(
      "_ZN4kush4data8PushBackEPNS0_6VectorE",
      program.PointerType(program.I8Type()), {struct_ptr});

  auto& get_fn = program.DeclareExternalFunction(
      "_ZN4kush4data3GetEPNS0_6VectorEj", program.PointerType(program.I8Type()),
      {struct_ptr, program.I32Type()});

  auto& size_fn = program.DeclareExternalFunction(
      "_ZN4kush4data4SizeEPNS0_6VectorE", program.I32Type(), {struct_ptr});

  auto& free_fn = program.DeclareExternalFunction(
      "_ZN4kush4data4FreeEPNS0_6VectorE", program.VoidType(), {struct_ptr});

  return ForwardDeclaredVectorFunctions<T>(struct_type, create_fn, push_back_fn,
                                           get_fn, size_fn, free_fn);
}

INSTANTIATE_ON_IR(Vector);

}  // namespace kush::compile::proxy