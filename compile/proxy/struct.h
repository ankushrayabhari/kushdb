#pragma once

#include <memory>
#include <vector>

#include "absl/types/span.h"

#include "catalog/sql_type.h"
#include "compile/program_builder.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

template <typename T>
class StructBuilder {
 public:
  StructBuilder(ProgramBuilder<T>& program);
  void Add(catalog::SqlType type);
  void Build();
  typename ProgramBuilder<T>::Type& Type();
  absl::Span<const catalog::SqlType> Types();
  absl::Span<const typename ProgramBuilder<T>::Value> DefaultValues();

 private:
  ProgramBuilder<T>& program_;
  std::vector<typename ProgramBuilder<T>::Type> fields_;
  std::vector<catalog::SqlType> types_;
  std::vector<typename ProgramBuilder<T>::Value> values_;
  std::optional<typename ProgramBuilder<T>::Type> struct_type_;
};

template <typename T>
class Struct {
 public:
  Struct(ProgramBuilder<T>& program, StructBuilder<T>& fields,
         const typename ProgramBuilder<T>::Value& value);

  void Pack(std::vector<std::reference_wrapper<proxy::Value<T>>> value);
  void Update(int field, const proxy::Value<T>& v);

  std::vector<std::unique_ptr<Value<T>>> Unpack();

 private:
  ProgramBuilder<T>& program_;
  StructBuilder<T>& fields_;
  typename ProgramBuilder<T>::Value value_;
};

}  // namespace kush::compile::proxy
