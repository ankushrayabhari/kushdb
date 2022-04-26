#pragma once

#include <cstdint>
#include <memory>

#include "runtime/tuple_idx_table/node.h"

namespace kush::runtime::TupleIdxTable {

class Node48 : public Node {
 public:
  Node48(Allocator& allocator, std::size_t compression_length);

  uint8_t child_index[256];
  Node* child[48];

 public:
  // Get position of a byte, returns -1 if not exists
  int32_t GetChildPos(uint8_t k) override;

  // Get the position of the first child that is greater or equal to the
  // specific byte, or-1 if there are no children
  // matching the criteria
  int32_t GetChildGreaterEqual(uint8_t k, bool& equal) override;

  // Get the next position in the node, or-1 if there
  // is no next position
  int32_t GetNextPos(int32_t pos) override;

  // Get Node48 Child
  Node** GetChild(int32_t pos) override;

  int32_t GetMin() override;

  // Insert node in Node48
  static void Insert(Node*& node, uint8_t key_byte, Node*& child);
};
}  // namespace kush::runtime::TupleIdxTable