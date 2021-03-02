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
  Vector(ProgramBuilder<T>& program, StructBuilder<T>& content);
  Vector(ProgramBuilder<T>& program, StructBuilder<T>& content,
         typename ProgramBuilder<T>::Value& value);

  Struct<T> operator[](const proxy::UInt32<T>& idx);
  Struct<T> PushBack();
  UInt32<T> Size();
  void Reset();
  void Sort(typename ProgramBuilder<T>::Function& comp);

  static void ForwardDeclare(ProgramBuilder<T>& program);

  static const std::string_view VectorStructName;
  static const std::string_view CreateFnName;
  static const std::string_view PushBackFnName;
  static const std::string_view GetFnName;
  static const std::string_view SizeFnName;
  static const std::string_view FreeFnName;
  static const std::string_view SortFnName;

 private:
  ProgramBuilder<T>& program_;
  StructBuilder<T>& content_;
  typename ProgramBuilder<T>::Type& content_type_;
  typename ProgramBuilder<T>::Value& value_;
};

template <typename T>
const std::string_view Vector<T>::VectorStructName("kush::data::Vector");

template <typename T>
const std::string_view Vector<T>::CreateFnName(
    "_ZN4kush4data6CreateEPNS0_6VectorEmj");

template <typename T>
const std::string_view Vector<T>::PushBackFnName(
    "_ZN4kush4data8PushBackEPNS0_6VectorE");

template <typename T>
const std::string_view Vector<T>::GetFnName("_ZN4kush4data3GetEPNS0_6VectorEj");

template <typename T>
const std::string_view Vector<T>::SizeFnName(
    "_ZN4kush4data4SizeEPNS0_6VectorE");

template <typename T>
const std::string_view Vector<T>::FreeFnName(
    "_ZN4kush4data4FreeEPNS0_6VectorE");

template <typename T>
const std::string_view Vector<T>::SortFnName(
    "_ZN4kush4data4SortEPNS0_6VectorEPFbPaS3_E");

}  // namespace kush::compile::proxy