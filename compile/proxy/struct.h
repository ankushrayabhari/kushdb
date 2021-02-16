#pragma once

#include <memory>
#include <vector>

#include "catalog/sql_type.h"
#include "compile/program_builder.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

template <typename T>
class StructBuilder {
 public:
  StructBuilder(ProgramBuilder<T>& program);
  void Add(catalog::SqlType type);
  typename ProgramBuilder<T>::Type& GenerateType();

  std::vector<std::reference_wrapper<typename ProgramBuilder<T>::Type>>
  Fields();

 private:
  ProgramBuilder<T>& program_;
  std::vector<std::reference_wrapper<typename ProgramBuilder<T>::Type>> fields_;
};

template <typename T>
class Struct {
 public:
  Struct(ProgramBuilder<T>& program, StructBuilder<T>& fields,
         typename ProgramBuilder<T>::Value& value);

  void Pack(std::vector<std::reference_wrapper<proxy::Value<T>>> value);

  std::vector<std::unique_ptr<Value<T>>> Unpack();

 private:
  ProgramBuilder<T>& program_;
  StructBuilder<T>& fields_;
  typename ProgramBuilder<T>::Value& value_;
};

}  // namespace kush::compile::proxy
