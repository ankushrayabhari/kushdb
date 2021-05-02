#pragma once

#include "compile/program_builder.h"
#include "compile/proxy/if.h"
#include "compile/proxy/int.h"
#include "compile/proxy/ptr.h"
#include "compile/proxy/struct.h"

namespace kush::compile::proxy {

template <typename T>
class Vector {
 public:
  Vector(ProgramBuilder<T>& program, StructBuilder<T>& content,
         bool global = false);
  Vector(ProgramBuilder<T>& program, StructBuilder<T>& content,
         const typename ProgramBuilder<T>::Value& value);

  Struct<T> operator[](const proxy::Int32<T>& idx);
  Struct<T> PushBack();
  Int32<T> Size();
  void Reset();
  void Sort(typename ProgramBuilder<T>::Function& comp);

  static void ForwardDeclare(ProgramBuilder<T>& program);

  static const std::string_view VectorStructName;

 private:
  ProgramBuilder<T>& program_;
  StructBuilder<T>& content_;
  typename ProgramBuilder<T>::Type content_type_;
  typename ProgramBuilder<T>::Value value_;
};

template <typename T>
const std::string_view Vector<T>::VectorStructName("kush::data::Vector");

}  // namespace kush::compile::proxy