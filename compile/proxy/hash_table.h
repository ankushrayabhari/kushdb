#pragma once

#include <functional>
#include <vector>

#include "compile/khir/khir_program_builder.h"
#include "compile/proxy/struct.h"
#include "compile/proxy/vector.h"

namespace kush::compile::proxy {

class HashTable {
 public:
  HashTable(khir::KHIRProgramBuilder& program, StructBuilder& content);
  ~HashTable();

  Struct Insert(std::vector<std::reference_wrapper<Value>> keys);
  Vector Get(std::vector<std::reference_wrapper<Value>> keys);
  void ForEach(std::function<void(Struct&)> handler);

  static void ForwardDeclare(khir::KHIRProgramBuilder& program);

 private:
  khir::KHIRProgramBuilder& program_;
  StructBuilder& content_;
  khir::Type content_type_;
  khir::Value value_;
  khir::Value hash_ptr_;
  khir::Value bucket_list_;
};

}  // namespace kush::compile::proxy