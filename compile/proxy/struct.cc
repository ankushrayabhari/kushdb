#include "compile/proxy/struct.h"

#include <memory>
#include <vector>

#include "absl/types/span.h"

#include "catalog/sql_type.h"
#include "compile/khir/program_builder.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/float.h"
#include "compile/proxy/int.h"
#include "compile/proxy/string.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

StructBuilder::StructBuilder(khir::ProgramBuilder& program)
    : program_(program) {}

void StructBuilder::Add(catalog::SqlType type) {
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
      fields_.push_back(program_.GetStructType(String::StringStructName));
      values_.push_back(String::Constant(program_, ""));
      break;
    case catalog::SqlType::BOOLEAN:
      fields_.push_back(program_.I1Type());
      values_.push_back(program_.ConstI1(0));
      break;
  }
}

void StructBuilder::Build() { struct_type_ = program_.StructType(fields_); }

khir::Type StructBuilder::Type() { return struct_type_.value(); }

absl::Span<const catalog::SqlType> StructBuilder::Types() { return types_; }

absl::Span<const khir::Value> StructBuilder::DefaultValues() { return values_; }

Struct::Struct(khir::ProgramBuilder& program, StructBuilder& fields,
               const khir::Value& value)
    : program_(program), fields_(fields), value_(value) {}

void Struct::Pack(std::vector<std::reference_wrapper<Value>> values) {
  auto types = fields_.Types();
  if (values.size() != types.size()) {
    throw std::runtime_error(
        "Must pack exactly number of values as fields in struct.");
  }

  for (int i = 0; i < values.size(); i++) {
    auto ptr = program_.GetElementPtr(fields_.Type(), value_, {0, i});
    auto v = values[i].get().Get();

    if (types[i] == catalog::SqlType::TEXT) {
      String rhs(program_, v);
      String(program_, ptr).Copy(rhs);
    } else {
      program_.Store(ptr, v);
    }
  }
}

std::vector<std::unique_ptr<Value>> Struct::Unpack() {
  auto types = fields_.Types();

  std::vector<std::unique_ptr<Value>> result;
  for (int i = 0; i < types.size(); i++) {
    auto ptr = program_.GetElementPtr(fields_.Type(), value_, {0, i});

    switch (types[i]) {
      case catalog::SqlType::SMALLINT:
        result.push_back(std::make_unique<Int16>(program_, program_.Load(ptr)));
        break;
      case catalog::SqlType::INT:
        result.push_back(std::make_unique<Int32>(program_, program_.Load(ptr)));
        break;
      case catalog::SqlType::BIGINT:
        result.push_back(std::make_unique<Int64>(program_, program_.Load(ptr)));
        break;
      case catalog::SqlType::DATE:
        result.push_back(std::make_unique<Int64>(program_, program_.Load(ptr)));
        break;
      case catalog::SqlType::REAL:
        result.push_back(
            std::make_unique<Float64>(program_, program_.Load(ptr)));
        break;
      case catalog::SqlType::TEXT:
        result.push_back(std::make_unique<String>(program_, ptr));
        break;
      case catalog::SqlType::BOOLEAN:
        result.push_back(std::make_unique<Bool>(program_, program_.Load(ptr)));
        break;
    }
  }
  return result;
}

void Struct::Update(int field, const proxy::Value& v) {
  auto types = fields_.Types();

  auto ptr = program_.GetElementPtr(fields_.Type(), value_, {0, field});

  if (types[field] == catalog::SqlType::TEXT) {
    String(program_, ptr).Copy(dynamic_cast<const String&>(v));
  } else {
    program_.Store(ptr, v.Get());
  }
}

}  // namespace kush::compile::proxy
