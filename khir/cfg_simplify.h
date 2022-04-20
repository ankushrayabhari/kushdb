#pragma once

#include <cstdint>
#include <vector>

#include "khir/program.h"

namespace kush::khir {

std::vector<BasicBlock> CFGSimplify(std::vector<uint64_t>& instrs,
                                    std::vector<BasicBlock> basic_blocks);

}  // namespace kush::khir