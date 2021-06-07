#pragma once

#include "compile/proxy/int.h"
#include "compile/proxy/struct.h"
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

  static void ForwardDeclare(khir::ProgramBuilder& program);

  static const std::string_view VectorStructName;

 private:
  khir::ProgramBuilder& program_;
  StructBuilder& content_;
  typename khir::Type content_type_;
  typename khir::Value value_;
};

}  // namespace kush::compile::proxy