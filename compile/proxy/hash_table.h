#pragma once

#include <functional>
#include <vector>

#include "compile/proxy/struct.h"
#include "compile/proxy/vector.h"
#include "execution/query_state.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

class HashTable {
 public:
  HashTable(khir::ProgramBuilder& program, execution::QueryState& state,
            StructBuilder& content);

  void Reset();
  Struct Insert(const std::vector<SQLValue>& keys);
  Vector Get(const std::vector<SQLValue>& keys);
  void ForEach(std::function<void(Struct&)> handler);

  static void ForwardDeclare(khir::ProgramBuilder& program);

 private:
  khir::ProgramBuilder& program_;
  StructBuilder& content_;
  khir::Type content_type_;
  khir::Value value_;
  khir::Value hash_ptr_;
  khir::Value bucket_list_;
};

}  // namespace kush::compile::proxy