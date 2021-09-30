#include "khir/backend.h"

#include "absl/flags/flag.h"

ABSL_FLAG(std::string, backend, "asm", "Compilation Backend: asm or llvm");

namespace kush::khir {

BackendType GetBackendType() {
  if (FLAGS_backend.CurrentValue() == "asm") {
    return BackendType::ASM;
  } else if (FLAGS_backend.CurrentValue() == "llvm") {
    return BackendType::LLVM;
  } else {
    throw std::runtime_error("Unknown backend.");
  }
}

}  // namespace kush::khir