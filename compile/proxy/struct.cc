#include "compile/proxy/struct.h"

#include <memory>
#include <vector>

#include "absl/types/span.h"

#include "catalog/sql_type.h"
#include "compile/proxy/control_flow/if.h"
#include "compile/proxy/value/ir_value.h"
#include "compile/proxy/value/sql_value.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

StructBuilder::StructBuilder(khir::ProgramBuilder& program)
    : program_(program) {}

int StructBuilder::Add(catalog::SqlType type, bool nullable) {
  auto field_idx = types_.size();
  field_to_struct_field_nullable_idx[field_idx] = {
      fields_.size(), nullable ? fields_.size() + 1 : -1};

  types_.push_back(type);
  nullable_.push_back(nullable);
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

  if (nullable) {
    fields_.push_back(program_.I8Type());
    values_.push_back(program_.ConstI8(0));
  }

  return field_idx;
}

std::pair<int, int> StructBuilder::GetFieldNullableIdx(int field) const {
  return field_to_struct_field_nullable_idx.at(field);
}

void StructBuilder::Build() { struct_type_ = program_.StructType(fields_); }

khir::Type StructBuilder::Type() const { return struct_type_.value(); }

absl::Span<const catalog::SqlType> StructBuilder::Types() const {
  return types_;
}

const std::vector<bool>& StructBuilder::Nullable() const { return nullable_; }

absl::Span<const khir::Value> StructBuilder::DefaultValues() const {
  return values_;
}

Struct::Struct(khir::ProgramBuilder& program, StructBuilder& fields,
               const khir::Value& value)
    : program_(program), fields_(fields), value_(value) {}

void Struct::Pack(const std::vector<SQLValue>& values) {
  auto types = fields_.Types();
  if (values.size() != types.size()) {
    throw std::runtime_error(
        "Must pack exactly number of values as fields in struct.");
  }

  for (int i = 0; i < values.size(); i++) {
    Update(i, values[i]);
  }
}

std::vector<SQLValue> Struct::Unpack() {
  auto types = fields_.Types();

  std::vector<SQLValue> result;
  for (int i = 0; i < types.size(); i++) {
    auto [field_idx, null_field_idx] = fields_.GetFieldNullableIdx(i);

    auto ptr = program_.GetElementPtr(fields_.Type(), value_, {0, field_idx});
    auto null =
        null_field_idx >= 0
            ? Int8(program_,
                   program_.LoadI8(program_.GetElementPtr(
                       fields_.Type(), value_, {0, null_field_idx}))) != 0
            : Bool(program_, false);

    std::unique_ptr<IRValue> value;
    switch (types[i]) {
      case catalog::SqlType::SMALLINT:
        result.emplace_back(Int16(program_, program_.LoadI16(ptr)), null);
        break;
      case catalog::SqlType::INT:
        result.emplace_back(Int32(program_, program_.LoadI32(ptr)), null);
        break;
      case catalog::SqlType::BIGINT:
      case catalog::SqlType::DATE:
        result.emplace_back(Int64(program_, program_.LoadI64(ptr)), null);
        break;
      case catalog::SqlType::REAL:
        result.emplace_back(Float64(program_, program_.LoadF64(ptr)), null);
        break;
      case catalog::SqlType::TEXT:
        result.emplace_back(String(program_, ptr), null);
        break;
      case catalog::SqlType::BOOLEAN:
        result.emplace_back(Int8(program_, program_.LoadI8(ptr)) != 0, null);
        break;
    }
  }
  return result;
}

void Struct::Store(catalog::SqlType t, khir::Value ptr, const IRValue& v) {
  auto value = v.Get();
  switch (t) {
    case catalog::SqlType::SMALLINT:
      program_.StoreI16(ptr, value);
      break;
    case catalog::SqlType::INT:
      program_.StoreI32(ptr, value);
      break;
    case catalog::SqlType::BIGINT:
    case catalog::SqlType::DATE:
      program_.StoreI64(ptr, value);
      break;
    case catalog::SqlType::REAL:
      program_.StoreF64(ptr, value);
      break;
    case catalog::SqlType::TEXT:
      String(program_, ptr).Copy(dynamic_cast<const String&>(v));
      break;
    case catalog::SqlType::BOOLEAN:
      program_.StoreI8(ptr, program_.I8ZextI1(value));
      break;
  }
}

void Struct::Update(int field, const SQLValue& v) {
  auto [f, nf] = fields_.GetFieldNullableIdx(field);
  int field_idx = f;
  int null_field_idx = nf;
  auto struct_type = fields_.Type();

  if (null_field_idx < 0) {
    auto ptr = program_.GetElementPtr(struct_type, value_, {0, field_idx});
    const auto& value = v.Get();
    auto type = v.Type();
    Store(type, ptr, value);
  } else {
    If(
        program_, v.IsNull(),
        [&]() {
          auto null_ptr =
              program_.GetElementPtr(struct_type, value_, {0, null_field_idx});
          program_.StoreI8(null_ptr, program_.ConstI8(1));
        },
        [&]() {
          auto null_ptr =
              program_.GetElementPtr(struct_type, value_, {0, null_field_idx});
          program_.StoreI8(null_ptr, program_.ConstI8(0));

          auto ptr =
              program_.GetElementPtr(struct_type, value_, {0, field_idx});
          const auto& value = v.Get();
          auto type = v.Type();
          Store(type, ptr, value);
        });
  }
}

}  // namespace kush::compile::proxy
