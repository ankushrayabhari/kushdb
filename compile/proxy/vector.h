#pragma once

#include "compile/proxy/struct.h"
#include "compile/proxy/value/ir_value.h"
#include "execution/query_state.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

class Vector {
 public:
  Vector(khir::ProgramBuilder& program, execution::QueryState& state,
         StructBuilder& content);
  Vector(khir::ProgramBuilder& program, StructBuilder& content, khir::Value v);

  Struct operator[](const Int32& idx);
  Struct PushBack();
  Int32 Size();
  void Init();
  void Reset();
  void Sort(const khir::FunctionRef& comp);

  khir::Value Get() const;

  static void ForwardDeclare(khir::ProgramBuilder& program);

  static constexpr std::string_view VectorStructName =
      "kush::runtime::Vector::Vector";

 private:
  khir::ProgramBuilder& program_;
  StructBuilder& content_;
  khir::Type content_type_;
  khir::Value value_;
};

}  // namespace kush::compile::proxy