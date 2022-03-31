#include "runtime/tuple_idx_table/node256.h"

#include "runtime/tuple_idx_table/node48.h"

namespace runtime::TupleIdxTable {

Node256::Node256(std::size_t compression_length)
    : Node(NodeType::N256, compression_length) {}

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

std::unique_ptr<Node>* Node256::GetChild(int32_t pos) {
  if (!child[pos]) {
    throw std::runtime_error("invalid child");
  }
  return &child[pos];
}

void Node256::Insert(std::unique_ptr<Node>& node, uint8_t key_byte,
                     std::unique_ptr<Node>& child) {
  Node256* n = static_cast<Node256*>(node.get());

  n->count++;
  n->child[key_byte] = std::move(child);
}

void Node256::Erase(std::unique_ptr<Node>& node, int pos) {
  Node256* n = static_cast<Node256*>(node.get());

  n->child[pos].reset();
  n->count--;
  if (node->count <= 36) {
    auto new_node = std::make_unique<Node48>(n->prefix_length);
    CopyPrefix(n, new_node.get());
    for (int32_t i = 0; i < 256; i++) {
      if (n->child[i]) {
        new_node->child_index[i] = new_node->count;
        new_node->child[new_node->count] = std::move(n->child[i]);
        new_node->count++;
      }
    }
    node = std::move(new_node);
  }
}

}  // namespace runtime::TupleIdxTable