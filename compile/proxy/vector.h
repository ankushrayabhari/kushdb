#pragma once

#include "compile/program_builder.h"
#include "compile/proxy/if.h"
#include "compile/proxy/int.h"
#include "compile/proxy/ptr.h"
#include "compile/proxy/struct.h"

namespace kush::compile::proxy {

template <typename T>
class ForwardDeclaredVectorFunctions {
 public:
  ForwardDeclaredVectorFunctions(
      typename ProgramBuilder<T>::Type& vector_type,
      typename ProgramBuilder<T>::Function& create_func,
      typename ProgramBuilder<T>::Function& push_back_func,
      typename ProgramBuilder<T>::Function& get_func,
      typename ProgramBuilder<T>::Function& size_func,
      typename ProgramBuilder<T>::Function& free_func,
      typename ProgramBuilder<T>::Function& sort_func);

  typename ProgramBuilder<T>::Type& VectorType();
  typename ProgramBuilder<T>::Function& Create();
  typename ProgramBuilder<T>::Function& PushBack();
  typename ProgramBuilder<T>::Function& Get();
  typename ProgramBuilder<T>::Function& Size();
  typename ProgramBuilder<T>::Function& Free();
  typename ProgramBuilder<T>::Function& Sort();

 private:
  typename ProgramBuilder<T>::Type& vector_type_;
  typename ProgramBuilder<T>::Function& create_func_;
  typename ProgramBuilder<T>::Function& push_back_func_;
  typename ProgramBuilder<T>::Function& get_func_;
  typename ProgramBuilder<T>::Function& size_func_;
  typename ProgramBuilder<T>::Function& free_func_;
  typename ProgramBuilder<T>::Function& sort_func_;
};

template <typename T>
class Vector {
 public:
  Vector(ProgramBuilder<T>& program,
         ForwardDeclaredVectorFunctions<T>& vector_funcs,
         StructBuilder<T>& content);
  ~Vector();

  Struct<T> operator[](const proxy::UInt32<T>& idx);
  Struct<T> PushBack();
  UInt32<T> Size();
  void Sort(typename ProgramBuilder<T>::Function& comp);

  static ForwardDeclaredVectorFunctions<T> ForwardDeclare(
      ProgramBuilder<T>& program);

 private:
  ProgramBuilder<T>& program_;
  ForwardDeclaredVectorFunctions<T> vector_funcs_;
  StructBuilder<T>& content_;
  typename ProgramBuilder<T>::Type& content_type_;
  typename ProgramBuilder<T>::Value& value_;
};

}  // namespace kush::compile::proxy