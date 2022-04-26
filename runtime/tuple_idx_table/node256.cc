#include "runtime/tuple_idx_table/node256.h"

#include <cstring>

#include "runtime/tuple_idx_table/node48.h"

namespace kush::runtime::TupleIdxTable {

Node256::Node256(Allocator& allocator, std::size_t compression_length)
    : Node(allocator, NodeType::N256, compression_length) {
  memset(child, 0, sizeof(child));
}

int32_t Node256::GetChildPos(uint8_t k) {
  if (child[k]) {
    return k;
  } else {
    return -1;
  }
}

int32_t Node256::GetChildGreaterEqual(uint8_t k, bool& equal) {
  for (int32_t pos = k; pos < 256; pos++) {
    if (child[pos]) {
      if (pos == k) {
        equal = true;
      } else {
        equal = false;
      }
      return pos;
    }
  }
  return -1;
}

int32_t Node256::GetMin() {
  for (int32_t i = 0; i < 256; i++) {
    if (child[i]) {
      return i;
    }
  }
  return -1;
}

int32_t Node256::GetNextPos(int32_t pos) {
  for (pos == -1 ? pos = 0 : pos++; pos < 256; pos++) {
    if (child[pos]) {
      return pos;
    }
  }
  return Node::GetNextPos(pos);
}

Node** Node256::GetChild(int32_t pos) {
  if (!child[pos]) {
    throw std::runtime_error("invalid child");
  }
  return &child[pos];
}

void Node256::Insert(Node*& node, uint8_t key_byte, Node*& child) {
  Node256* n = static_cast<Node256*>(node);

  n->count++;
  n->child[key_byte] = std::move(child);
}

}  // namespace kush::runtime::TupleIdxTable