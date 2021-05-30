#include "compile/khir/asm/asm_backend.h"

#include "absl/types/span.h"

#include "asmjit/x86.h"

#include "compile/khir/instruction.h"
#include "compile/khir/program_builder.h"
#include "compile/khir/type_manager.h"

namespace kush::khir {

// Instructions
void ASMBackend::TranslateFuncDecl(bool pub, bool external,
                                   std::string_view name, Type function_type) {}

void ASMBackend::TranslateFuncBody(
    int func_idx, const std::vector<uint64_t>& i64_constants,
    const std::vector<double>& f64_constants,
    const std::vector<int>& basic_block_order,
    const std::vector<std::pair<int, int>>& basic_blocks,
    const std::vector<uint64_t>& instructions) {}

}  // namespace kush::khir