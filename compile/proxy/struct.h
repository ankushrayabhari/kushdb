#pragma once

#include <memory>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/types/span.h"

#include "catalog/sql_type.h"
#include "compile/proxy/value/sql_value.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

class StructBuilder {
 public:
  StructBuilder(khir::ProgramBuilder& program);
  void Add(catalog::SqlType type, bool nullable);
  void Build();

  khir::Type Type() const;
  absl::Span<const catalog::SqlType> Types() const;
  absl::Span<const khir::Value> DefaultValues() const;
  std::pair<int, int> GetFieldNullableIdx(int field) const;

 private:
  khir::ProgramBuilder& program_;
  std::vector<khir::Type> fields_;
  std::vector<catalog::SqlType> types_;
  std::vector<khir::Value> values_;
  std::optional<khir::Type> struct_type_;
  absl::flat_hash_map<int, std::pair<int, int>>
      field_to_struct_field_nullable_idx;
};

class Struct {
 public:
  Struct(khir::ProgramBuilder& program, StructBuilder& fields,
         const khir::Value& value);

  void Pack(std::vector<std::reference_wrapper<SQLValue>> value);
  void Update(int field, const SQLValue& v);
  std::vector<SQLValue> Unpack();

 private:
  khir::ProgramBuilder& program_;
  StructBuilder& fields_;
  khir::Value value_;
};

}  // namespace kush::compile::proxy
