#include "compile/proxy/struct.h"

#include <memory>
#include <vector>

#include "absl/types/span.h"

#include "catalog/sql_type.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/float.h"
#include "compile/proxy/int.h"
#include "compile/proxy/string.h"
#include "compile/proxy/value.h"
#include "khir/program_builder.h"

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
      fields_.push_back(program_.I8Type());
      values_.push_back(program_.ConstI8(0));
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
    Update(i, values[i].get());
  }
}

std::vector<std::unique_ptr<Value>> Struct::Unpack() {
  auto types = fields_.Types();

  std::vector<std::unique_ptr<Value>> result;
  for (int i = 0; i < types.size(); i++) {
    auto ptr = program_.GetElementPtr(fields_.Type(), value_, {0, i});

    switch (types[i]) {
      case catalog::SqlType::SMALLINT:
        result.push_back(
            std::make_unique<Int16>(program_, program_.LoadI16(ptr)));
        break;
      case catalog::SqlType::INT:
        result.push_back(
            std::make_unique<Int32>(program_, program_.LoadI32(ptr)));
        break;
      case catalog::SqlType::BIGINT:
      case catalog::SqlType::DATE:
        result.push_back(
            std::make_unique<Int64>(program_, program_.LoadI64(ptr)));
        break;
      case catalog::SqlType::REAL:
        result.push_back(
            std::make_unique<Float64>(program_, program_.LoadF64(ptr)));
        break;
      case catalog::SqlType::TEXT:
        result.push_back(std::make_unique<String>(program_, ptr));
        break;
      case catalog::SqlType::BOOLEAN:
        result.push_back(
            (proxy::Int8(program_, program_.LoadI8(ptr)) == 0).ToPointer());
        break;
    }
  }
  return result;
}

void Struct::Update(int field, const proxy::Value& v) {
  auto types = fields_.Types();

  auto ptr = program_.GetElementPtr(fields_.Type(), value_, {0, field});

  switch (types[field]) {
    case catalog::SqlType::SMALLINT:
      program_.StoreI16(ptr, v.Get());
      break;
    case catalog::SqlType::INT:
      program_.StoreI32(ptr, v.Get());
      break;
    case catalog::SqlType::BIGINT:
    case catalog::SqlType::DATE:
      program_.StoreI64(ptr, v.Get());
      break;
    case catalog::SqlType::REAL:
      program_.StoreF64(ptr, v.Get());
      break;
    case catalog::SqlType::TEXT:
      String(program_, ptr).Copy(dynamic_cast<const String&>(v));
      break;
    case catalog::SqlType::BOOLEAN:
      program_.StoreI8(ptr, program_.I8ZextI1(v.Get()));
      break;
  }
}

}  // namespace kush::compile::proxy
