#include "runtime/tuple_idx_table/leaf.h"

#include <cstdint>
#include <memory>

#include "runtime/tuple_idx_table/node.h"

namespace kush::runtime::TupleIdxTable {

Leaf::Leaf(Allocator& allocator, Key* v)
    : Node(allocator, NodeType::NLeaf, 0), value(v) {}

}  // namespace kush::runtime::TupleIdxTable