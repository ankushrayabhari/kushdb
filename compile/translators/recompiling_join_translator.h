#pragma once

#include <cstdint>
#include <vector>

namespace kush::compile {

class RecompilingJoinTranslator {
 public:
  typedef int32_t (*ExecuteJoinFn)(int32_t initial_budget);
  virtual ExecuteJoinFn CompileJoinOrder(const std::vector<int>& order,
                                         void** materialized_buffers,
                                         void** materialized_indexes,
                                         void* tuple_idx_table) = 0;
};

}  // namespace kush::compile