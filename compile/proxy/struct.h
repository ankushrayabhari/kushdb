#pragma once

#include <memory>
#include <vector>

#include "absl/types/span.h"

#include "catalog/sql_type.h"
#include "compile/khir/program_builder.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

class StructBuilder {
 public:
  StructBuilder(khir::ProgramBuilder& program);
  void Add(catalog::SqlType type);
  void Build();
  khir::Type Type();
  absl::Span<const catalog::SqlType> Types();
  absl::Span<const khir::Value> DefaultValues();

 private:
  khir::ProgramBuilder& program_;
  std::vector<khir::Type> fields_;
  std::vector<catalog::SqlType> types_;
  std::vector<khir::Value> values_;
  std::optional<khir::Type> struct_type_;
};

class Struct {
 public:
  Struct(khir::ProgramBuilder& program, StructBuilder& fields,
         const khir::Value& value);

  void Pack(std::vector<std::reference_wrapper<proxy::Value>> value);
  void Update(int field, const proxy::Value& v);

  std::vector<std::unique_ptr<Value>> Unpack();

 private:
  khir::ProgramBuilder& program_;
  StructBuilder& fields_;
  khir::Value value_;
};

}  // namespace kush::compile::proxy
