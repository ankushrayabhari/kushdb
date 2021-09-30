#pragma once

namespace kush::khir {

enum class BackendType { ASM, LLVM };

BackendType GetBackendType();

}  // namespace kush::khir