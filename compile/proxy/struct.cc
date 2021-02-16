#include "compile/proxy/struct.h"

#include <memory>
#include <vector>

#include "catalog/sql_type.h"
#include "compile/ir_registry.h"
#include "compile/program_builder.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

template <typename T>
StructBuilder<T>::StructBuilder(ProgramBuilder<T>& program)
    : program_(program) {}

template <typename T>
void StructBuilder<T>::Add(catalog::SqlType type) {
  switch (type) {
    case catalog::SqlType::SMALLINT:
      fields_.push_back(program_.I16Type());
      break;
    case catalog::SqlType::INT:
      fields_.push_back(program_.I32Type());
      break;
    case catalog::SqlType::BIGINT:
      fields_.push_back(program_.I64Type());
      break;
    case catalog::SqlType::REAL:
      fields_.push_back(program_.F64Type());
      break;
    case catalog::SqlType::DATE:
      fields_.push_back(program_.I64Type());
      break;
    case catalog::SqlType::TEXT:
      throw std::runtime_error("Text not supported at the moment.");
      break;
    case catalog::SqlType::BOOLEAN:
      fields_.push_back(program_.I8Type());
      break;
  }
}

template <typename T>
typename ProgramBuilder<T>::Type& StructBuilder<T>::GenerateType() {
  return program_.StructType(fields_);
}

template <typename T>
std::vector<std::reference_wrapper<typename ProgramBuilder<T>::Type>>
StructBuilder<T>::Fields() {
  return fields_;
}

INSTANTIATE_ON_IR(StructBuilder);

template <typename T>
Struct<T>::Struct(ProgramBuilder<T>& program, StructBuilder<T>& fields,
                  typename ProgramBuilder<T>::Value& value)
    : program_(program), fields_(fields), value_(value) {}

template <typename T>
void Struct<T>::Pack(
    std::vector<std::reference_wrapper<proxy::Value<T>>> value) {
  // TODO:
}

template <typename T>
std::vector<std::unique_ptr<Value<T>>> Struct<T>::Unpack() {
  // TODO:
  return {};
}

INSTANTIATE_ON_IR(Struct);

}  // namespace kush::compile::proxy
