#include "compile/proxy/struct.h"

#include <memory>
#include <vector>

#include "catalog/sql_type.h"
#include "compile/ir_registry.h"
#include "compile/program_builder.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/float.h"
#include "compile/proxy/int.h"
#include "compile/proxy/string.h"
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
      values_.push_back(program_.ConstI16(0));
      break;
    case catalog::SqlType::INT:
      fields_.push_back(program_.I32Type());
      values_.push_back(program_.ConstI32(0));
      break;
    case catalog::SqlType::BIGINT:
      fields_.push_back(program_.I64Type());
      values_.push_back(program_.ConstI64(0));
      break;
    case catalog::SqlType::REAL:
      fields_.push_back(program_.F64Type());
      values_.push_back(program_.ConstF64(0));
      break;
    case catalog::SqlType::DATE:
      fields_.push_back(program_.I64Type());
      values_.push_back(program_.ConstI64(0));
      break;
    case catalog::SqlType::TEXT:
      fields_.push_back(program_.GetStructType(String<T>::StringStructName));
      values_.push_back(String<T>::Constant(program_, "").Get());
      break;
    case catalog::SqlType::BOOLEAN:
      fields_.push_back(program_.I1Type());
      values_.push_back(program_.ConstI1(0));
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

template <typename T>
std::vector<std::reference_wrapper<typename ProgramBuilder<T>::Value>>
StructBuilder<T>::DefaultValues() {
  return values_;
}

INSTANTIATE_ON_IR(StructBuilder);

template <typename T>
Struct<T>::Struct(ProgramBuilder<T>& program, StructBuilder<T>& fields,
                  typename ProgramBuilder<T>::Value& value)
    : program_(program), fields_(fields), value_(value) {}

template <typename T>
void Struct<T>::Pack(std::vector<std::reference_wrapper<Value<T>>> values) {
  auto types = fields_.Types();
  if (values.size() != types.size()) {
    throw std::runtime_error(
        "Must pack exactly number of values as fields in struct.");
  }

  for (int i = 0; i < values.size(); i++) {
    auto& ptr = program_.GetElementPtr(
        fields_.Type(), value_, {program_.ConstI32(0), program_.ConstI32(i)});
    auto& v = values[i].get().Get();

    if (types[i] == catalog::SqlType::TEXT) {
      String<T> rhs(program_, v);
      String<T>(program_, ptr).Copy(rhs);
    } else {
      program_.Store(ptr, v);
    }
  }
}

template <typename T>
std::vector<std::unique_ptr<Value<T>>> Struct<T>::Unpack() {
  auto types = fields_.Types();

  std::vector<std::unique_ptr<Value<T>>> result;
  for (int i = 0; i < types.size(); i++) {
    auto& ptr = program_.GetElementPtr(
        fields_.Type(), value_, {program_.ConstI32(0), program_.ConstI32(i)});

    switch (types[i]) {
      case catalog::SqlType::SMALLINT:
        result.push_back(
            std::make_unique<Int16<T>>(program_, program_.Load(ptr)));
        break;
      case catalog::SqlType::INT:
        result.push_back(
            std::make_unique<Int32<T>>(program_, program_.Load(ptr)));
        break;
      case catalog::SqlType::BIGINT:
        result.push_back(
            std::make_unique<Int64<T>>(program_, program_.Load(ptr)));
        break;
      case catalog::SqlType::DATE:
        result.push_back(
            std::make_unique<Int64<T>>(program_, program_.Load(ptr)));
        break;
      case catalog::SqlType::REAL:
        result.push_back(
            std::make_unique<Float64<T>>(program_, program_.Load(ptr)));
        break;
      case catalog::SqlType::TEXT:
        result.push_back(std::make_unique<String<T>>(program_, ptr));
        break;
      case catalog::SqlType::BOOLEAN:
        result.push_back(
            std::make_unique<Bool<T>>(program_, program_.Load(ptr)));
        break;
    }
  }
  return result;
}

template <typename T>
void Struct<T>::Update(int field, proxy::Value<T>& v) {
  auto types = fields_.Types();

  auto& ptr = program_.GetElementPtr(
      fields_.Type(), value_, {program_.ConstI32(0), program_.ConstI32(field)});

  if (types[field] == catalog::SqlType::TEXT) {
    String<T>(program_, ptr).Copy(dynamic_cast<String<T>&>(v));
  } else {
    program_.Store(ptr, v.Get());
  }
}

INSTANTIATE_ON_IR(Struct);

}  // namespace kush::compile::proxy
