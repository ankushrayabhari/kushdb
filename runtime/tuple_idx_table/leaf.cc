#include "runtime/tuple_idx_table/leaf.h"

#include <cstdint>
#include <memory>

#include "runtime/tuple_idx_table/node.h"

namespace kush::runtime::TupleIdxTable {

Leaf::Leaf(std::unique_ptr<Key> v)
    : Node(NodeType::NLeaf, 0), value(std::move(v)) {}

}  // namespace kush::runtime::TupleIdxTable