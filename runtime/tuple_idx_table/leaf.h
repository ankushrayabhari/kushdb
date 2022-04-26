#pragma once

#include <cstdint>
#include <memory>

#include "runtime/tuple_idx_table/node.h"

namespace kush::runtime::TupleIdxTable {

class Leaf : public Node {
 public:
  Leaf(Allocator& allocator, Key* value);

  Key* value;
};

}  // namespace kush::runtime::TupleIdxTable