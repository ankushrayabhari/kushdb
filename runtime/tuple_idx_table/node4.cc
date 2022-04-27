#include "runtime/tuple_idx_table/node4.h"

#include <cstring>
#include <memory>

#include "runtime/tuple_idx_table/node16.h"

namespace kush::runtime::TupleIdxTable {

Node4::Node4(Allocator& allocator, std::size_t compression_length)
    : Node(allocator, NodeType::N4, compression_length) {
  memset(key, 0, sizeof(key));
  memset(child, 0, sizeof(child));
}

int32_t Node4::GetChildPos(uint8_t k) {
  for (int32_t pos = 0; pos < count; pos++) {
    if (key[pos] == k) {
      return pos;
    }
  }
  return Node::GetChildPos(k);
}

int32_t Node4::GetChildGreaterEqual(uint8_t k, bool& equal) {
  for (int32_t pos = 0; pos < count; pos++) {
    if (key[pos] >= k) {
      if (key[pos] == k) {
        equal = true;
      } else {
        equal = false;
      }
      return pos;
    }
  }
  return Node::GetChildGreaterEqual(k, equal);
}

int32_t Node4::GetMin() { return 0; }

int32_t Node4::GetNextPos(int32_t pos) {
  if (pos == -1) {
    return 0;
  }
  pos++;
  return pos < count ? pos : -1;
}

Node** Node4::GetChild(int32_t pos) {
  if (pos >= count) {
    throw std::runtime_error("invalid pos");
  }

  return &child[pos];
}

void Node4::Insert(Node*& node, uint8_t key_byte, Node*& child) {
  Node4* n = static_cast<Node4*>(node);

  // Insert leaf into inner node
  if (node->count < 4) {
    // Insert element
    int32_t pos = 0;
    while ((pos < node->count) && (n->key[pos] < key_byte)) {
      pos++;
    }
    if (n->child[pos] != nullptr) {
      for (int32_t i = n->count; i > pos; i--) {
        n->key[i] = n->key[i - 1];
        n->child[i] = std::move(n->child[i - 1]);
      }
    }
    n->key[pos] = key_byte;
    n->child[pos] = std::move(child);
    n->count++;
  } else {
    // Grow to Node16
    auto new_node =
        n->allocator.Allocate<Node16>(n->allocator, n->prefix_length);
    new_node->count = 4;
    CopyPrefix(node, new_node);
    for (int32_t i = 0; i < 4; i++) {
      new_node->key[i] = n->key[i];
      new_node->child[i] = std::move(n->child[i]);
    }
    node = std::move(new_node);
    Node16::Insert(node, key_byte, child);
  }
}

}  // namespace kush::runtime::TupleIdxTable