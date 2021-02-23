#include "compile/proxy/struct.h"

#include <memory>
#include <vector>

#include "catalog/sql_type.h"
#include "compile/ir_registry.h"
#include "compile/program_builder.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/float.h"
#include "compile/proxy/int.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

template <typename T>
StructBuilder<T>::StructBuilder(ProgramBuilder<T>& program)
    : program_(program), struct_type_(nullptr) {}

template <typename T>
void StructBuilder<T>::Add(catalog::SqlType type) {
  types_.push_back(type);
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
void StructBuilder<T>::Build() {
  struct_type_ = &program_.StructType(fields_);
}

template <typename T>
typename ProgramBuilder<T>::Type& StructBuilder<T>::Type() {
  return *struct_type_;
}

template <typename T>
std::vector<catalog::SqlType> StructBuilder<T>::Types() {
  return types_;
}

INSTANTIATE_ON_IR(StructBuilder);

template <typename T>
Struct<T>::Struct(ProgramBuilder<T>& program, StructBuilder<T>& fields,
                  typename ProgramBuilder<T>::Value& value)
    : program_(program), fields_(fields), value_(value) {}

template <typename T>
void Struct<T>::Pack(std::vector<std::reference_wrapper<Value<T>>> values) {
  auto types = fields_.Types();
  assert(values.size() == types.size());

  for (int i = 0; i < values.size(); i++) {
    auto& ptr = program_.GetElementPtr(
        fields_.Type(), value_, {program_.ConstI32(0), program_.ConstI32(i)});

    if (types[i] == catalog::SqlType::TEXT) {
      throw std::runtime_error("Text not supported at the moment.");
    }

    auto& value = values[i].get().Get();

    program_.Store(ptr, value);
  }
}

template <typename T>
std::vector<std::unique_ptr<Value<T>>> Struct<T>::Unpack() {
  auto types = fields_.Types();

  std::vector<std::unique_ptr<Value<T>>> result;
  for (int i = 0; i < types.size(); i++) {
    auto& ptr = program_.GetElementPtr(
        fields_.Type(), value_, {program_.ConstI32(0), program_.ConstI32(i)});
    auto& value = program_.Load(ptr);

    switch (types[i]) {
      case catalog::SqlType::SMALLINT:
        result.push_back(std::make_unique<Int16<T>>(program_, value));
        break;
      case catalog::SqlType::INT:
        result.push_back(std::make_unique<Int32<T>>(program_, value));
        break;
      case catalog::SqlType::BIGINT:
        result.push_back(std::make_unique<Int64<T>>(program_, value));
        break;
      case catalog::SqlType::DATE:
        result.push_back(std::make_unique<Int64<T>>(program_, value));
        break;
      case catalog::SqlType::REAL:
        result.push_back(std::make_unique<Float64<T>>(program_, value));
        break;
      case catalog::SqlType::TEXT:
        throw std::runtime_error("Text unsupported at the moment.");
        break;
      case catalog::SqlType::BOOLEAN:
        result.push_back(std::make_unique<Bool<T>>(program_, value));
        break;
    }
  }
  return result;
}

template <typename T>
void Struct<T>::Update(proxy::Value<T>& v, int field) {
  auto types = fields_.Types();

  auto& ptr = program_.GetElementPtr(
      fields_.Type(), value_, {program_.ConstI32(0), program_.ConstI32(field)});

  if (types[field] == catalog::SqlType::TEXT) {
    throw std::runtime_error("Text not supported at the moment.");
  }

  program_.Store(ptr, v.Get());
}

INSTANTIATE_ON_IR(Struct);

}  // namespace kush::compile::proxy
