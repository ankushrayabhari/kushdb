#pragma once

#include "compile/khir/khir_program_builder.h"
#include "compile/khir/type_manager.h"

namespace kush::khir {

class KhirProgramTranslator : public TypeTranslator {
 public:
  virtual void TranslateFuncDecl(bool external, std::string_view name,
                                 Type function_type) = 0;
  virtual void TranslateFuncBody(
      int func_idx, const std::vector<uint64_t>& i64_constants,
      const std::vector<double>& f64_constants,
      const std::vector<int>& basic_block_order,
      const std::vector<std::pair<int, int>>& basic_blocks,
      const std::vector<uint64_t>& instructions) = 0;
};

}  // namespace kush::khir