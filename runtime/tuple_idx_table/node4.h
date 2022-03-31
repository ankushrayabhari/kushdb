#pragma once

#include <cstdint>
#include <memory>

#include "runtime/tuple_idx_table/node.h"

namespace runtime::TupleIdxTable {

class Node4 : public Node {
 public:
  Node4(std::size_t compression_length);

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
  std::unique_ptr<Node>* GetChild(int32_t pos) override;

  int32_t GetMin() override;

  // Insert Leaf to the Node4
  static void Insert(std::unique_ptr<Node>& node, uint8_t key_byte,
                     std::unique_ptr<Node>& child);

  // Remove Leaf from Node4
  static void Erase(std::unique_ptr<Node>& node, int pos);

  uint8_t key[4];
  std::unique_ptr<Node> child[4];
};

}  // namespace runtime::TupleIdxTable