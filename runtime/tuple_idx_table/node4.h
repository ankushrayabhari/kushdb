#pragma once

#include <cstdint>
#include <memory>

#include "runtime/tuple_idx_table/node.h"

namespace kush::runtime::TupleIdxTable {

class Node4 : public Node {
 public:
  Node4(Allocator& allocator, std::size_t compression_length);

  // Get position of a byte, returns -1 if not exists
  int32_t GetChildPos(uint8_t k) override;

  // Get the position of the first child that is greater or equal to the
  // specific byte, or -1 if there are no children
  // matching the criteria
  int32_t GetChildGreaterEqual(uint8_t k, bool& equal) override;

  // Get the next position in the node, or -1 if there
  // is no next position
  int32_t GetNextPos(int32_t pos) override;

  // Get Node4 Child
  Node** GetChild(int32_t pos) override;

  int32_t GetMin() override;

  // Insert Leaf to the Node4
  static void Insert(Node*& node, uint8_t key_byte, Node*& child);

  uint8_t key[4];
  Node* child[4];
};

}  // namespace kush::runtime::TupleIdxTable