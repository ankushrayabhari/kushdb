#pragma once

#include <vector>

namespace kush::compile {

class RecompilingJoinTranslator {
 public:
  virtual void* CompileJoinOrder(const std::vector<int>& order) = 0;
};

}  // namespace kush::compile