#pragma once

#include <string_view>

#include "khir/program.h"

namespace kush::khir {

class Backend {
 public:
  virtual ~Backend() = default;
  virtual void* GetFunction(std::string_view name) = 0;
};

enum class BackendType { ASM, LLVM };

BackendType GetBackendType();

}  // namespace kush::khir