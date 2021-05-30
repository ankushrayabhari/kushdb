#pragma once

#include "compile/khir/program_builder.h"
#include "compile/proxy/if.h"
#include "compile/proxy/int.h"
#include "compile/proxy/ptr.h"
#include "compile/proxy/struct.h"

namespace kush::compile::proxy {

class GlobalVector;

class Vector {
 public:
  Vector(khir::ProgramBuilder& program, StructBuilder& content);
  Vector(khir::ProgramBuilder& program, StructBuilder& content,
         khir::Value value);

  Struct operator[](const proxy::Int32& idx);
  Struct PushBack();
  Int32 Size();
  void Reset();
  void Sort(const khir::FunctionRef& comp);

  static void ForwardDeclare(khir::ProgramBuilder& program);

  static const std::string_view VectorStructName;

 private:
  void Init();
  friend class GlobalVector;

  khir::ProgramBuilder& program_;
  StructBuilder& content_;
  typename khir::Type content_type_;
  typename khir::Value value_;
};

class GlobalVector {
 public:
  GlobalVector(khir::ProgramBuilder& program, StructBuilder& content);
  Vector Get();

 public:
  khir::ProgramBuilder& program_;
  StructBuilder& content_;
  std::function<khir::Value()> generator_;
};

}  // namespace kush::compile::proxy