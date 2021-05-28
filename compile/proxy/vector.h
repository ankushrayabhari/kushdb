#pragma once

#include "compile/khir/khir_program_builder.h"
#include "compile/proxy/if.h"
#include "compile/proxy/int.h"
#include "compile/proxy/ptr.h"
#include "compile/proxy/struct.h"

namespace kush::compile::proxy {

class Vector {
 public:
  Vector(khir::KHIRProgramBuilder& program, StructBuilder& content,
         bool global = false);
  Vector(khir::KHIRProgramBuilder& program, StructBuilder& content,
         const khir::Value& value);

  Struct operator[](const proxy::Int32& idx);
  Struct PushBack();
  Int32 Size();
  void Reset();
  void Sort(const khir::FunctionRef& comp);

  static void ForwardDeclare(khir::KHIRProgramBuilder& program);

  static const std::string_view VectorStructName;

 private:
  khir::KHIRProgramBuilder& program_;
  StructBuilder& content_;
  typename khir::Type content_type_;
  typename khir::Value value_;
};

const std::string_view Vector::VectorStructName("kush::data::Vector");

}  // namespace kush::compile::proxy