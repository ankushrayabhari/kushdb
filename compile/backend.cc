#include "compile/backend.h"

#include "absl/flags/flag.h"

ABSL_FLAG(std::string, backend, "asm", "Compilation Backend: asm or llvm");

namespace kush::compile {

Backend GetBackend() {
  if (FLAGS_backend.CurrentValue() == "asm") {
    return Backend::ASM;
  } else if (FLAGS_backend.CurrentValue() == "llvm") {
    return Backend::LLVM;
  } else {
    throw std::runtime_error("Unknown backend.");
  }
}

}  // namespace kush::compile