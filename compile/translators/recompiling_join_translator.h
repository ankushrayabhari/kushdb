#pragma once

#include <vector>

namespace kush::compile {

class RecompilingJoinTranslator {
 public:
  typedef void (*ExecuteJoinFn)();
  virtual ExecuteJoinFn CompileJoinOrder(const std::vector<int>& order,
                                         void** materialized_buffers,
                                         void** materialized_indexes,
                                         void* tuple_idx_table) = 0;
};

}  // namespace kush::compile