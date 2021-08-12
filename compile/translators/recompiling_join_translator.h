#pragma once

#include <vector>

namespace kush::compile {

class RecompilingJoinTranslator {
 public:
  typedef void (*ExecuteJoinFn)();
  virtual ExecuteJoinFn CompileJoinOrder(const std::vector<int>& order,
                                         void** materialized_buffers) = 0;
};

}  // namespace kush::compile