#include "runtime/tuple_idx_table/node48.h"

#include <cstring>

#include "runtime/tuple_idx_table/node16.h"
#include "runtime/tuple_idx_table/node256.h"

namespace kush::runtime::TupleIdxTable {

Node48::Node48(Allocator& allocator, std::size_t compression_length)
    : Node(allocator, NodeType::N48, compression_length) {
  for (int32_t i = 0; i < 256; i++) {
    child_index[i] = Node::EMPTY_MARKER;
  }
  memset(child, 0, sizeof(child));
}

int32_t Node48::GetChildPos(uint8_t k) {
  if (child_index[k] == Node::EMPTY_MARKER) {
    return -1;
  } else {
    return k;
  }
}

int32_t Node48::GetChildGreaterEqual(uint8_t k, bool& equal) {
  for (int32_t pos = k; pos < 256; pos++) {
    if (child_index[pos] != Node::EMPTY_MARKER) {
      if (pos == k) {
        equal = true;
      } else {
        equal = false;
      }
      return pos;
    }
  }
  return Node::GetChildGreaterEqual(k, equal);
}

int32_t Node48::GetNextPos(int32_t pos) {
  for (pos == -1 ? pos = 0 : pos++; pos < 256; pos++) {
    if (child_index[pos] != Node::EMPTY_MARKER) {
      return pos;
    }
  }
  return Node::GetNextPos(pos);
}

Node** Node48::GetChild(int32_t pos) {
  if (child_index[pos] == Node::EMPTY_MARKER) {
    throw std::runtime_error("Invalid");
  }

  return &child[child_index[pos]];
}

int32_t Node48::GetMin() {
  for (int32_t i = 0; i < 256; i++) {
    if (child_index[i] != Node::EMPTY_MARKER) {
      return i;
    }
  }
  return -1;
}

void Node48::Insert(Node*& node, uint8_t key_byte, Node*& child) {
  Node48* n = static_cast<Node48*>(node);

  // Insert leaf into inner node
  if (node->count < 48) {
    // Insert element
    int32_t pos = n->count;
    if (n->child[pos]) {
      // find an empty position in the node list if the current position is
      // occupied
      pos = 0;
      while (n->child[pos]) {
        pos++;
      }
    }
    n->child[pos] = std::move(child);
    n->child_index[key_byte] = pos;
    n->count++;
  } else {
    // Grow to Node256
    auto new_node =
        n->allocator.Allocate<Node256>(n->allocator, n->prefix_length);
    for (int32_t i = 0; i < 256; i++) {
      if (n->child_index[i] != Node::EMPTY_MARKER) {
        new_node->child[i] = std::move(n->child[n->child_index[i]]);
      }
    }
    new_node->count = n->count;
    CopyPrefix(n, new_node);
    node = std::move(new_node);
    Node256::Insert(node, key_byte, child);
  }
}

}  // namespace kush::runtime::TupleIdxTable