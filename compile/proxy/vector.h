#pragma once

#include "compile/proxy/struct.h"
#include "compile/proxy/value/value.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

class GlobalVector;

class Vector {
 public:
  Vector(khir::ProgramBuilder& program, StructBuilder& content, bool global);
  Vector(khir::ProgramBuilder& program, StructBuilder& content, khir::Value v);

  Struct operator[](const proxy::Int32& idx);
  Struct PushBack();
  Int32 Size();
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