#include "runtime/tuple_idx_table/node16.h"

#include <cstring>

#include "runtime/tuple_idx_table/node4.h"
#include "runtime/tuple_idx_table/node48.h"

namespace kush::runtime::TupleIdxTable {

Node16::Node16(Allocator &allocator, std::size_t compression_length)
    : Node(allocator, NodeType::N16, compression_length) {
  memset(key, 16, sizeof(key));
  memset(child, 0, sizeof(child));
}

int32_t Node16::GetChildPos(uint8_t k) {
  for (int32_t pos = 0; pos < count; pos++) {
    if (key[pos] == k) {
      return pos;
    }
  }
  return Node::GetChildPos(k);
}

int32_t Node16::GetChildGreaterEqual(uint8_t k, bool &equal) {
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

int32_t Node16::GetNextPos(int32_t pos) {
  if (pos == -1) {
    return 0;
  }
  pos++;
  return pos < count ? pos : -1;
}

Node **Node16::GetChild(int32_t pos) { return &child[pos]; }

int32_t Node16::GetMin() { return 0; }

void Node16::Insert(Node *&node, uint8_t key_byte, Node *&child) {
  Node16 *n = static_cast<Node16 *>(node);

  if (n->count < 16) {
    // Insert element
    int32_t pos = 0;
    while (pos < node->count && n->key[pos] < key_byte) {
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
    // Grow to Node48
    auto dest = n->allocator.AllocateData(sizeof(Node48));
    auto new_node = new (dest) Node48(n->allocator, n->prefix_length);
    for (int32_t i = 0; i < node->count; i++) {
      new_node->child_index[n->key[i]] = i;
      new_node->child[i] = std::move(n->child[i]);
    }
    CopyPrefix(n, new_node);
    new_node->count = node->count;
    node = std::move(new_node);

    Node48::Insert(node, key_byte, child);
  }
}

}  // namespace kush::runtime::TupleIdxTable