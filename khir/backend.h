#pragma once

#include <string_view>

#include "khir/program.h"

namespace kush::khir {

class Backend : public ProgramTranslator {
 public:
  virtual ~Backend() = default;
  virtual void Compile() = 0;
  virtual void* GetFunction(std::string_view name) const = 0;
};

enum class BackendType { ASM, LLVM };

BackendType GetBackendType();

}  // namespace kush::khir