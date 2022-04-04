#include "runtime/tuple_idx_table/node48.h"

#include "runtime/tuple_idx_table/node16.h"
#include "runtime/tuple_idx_table/node256.h"

namespace kush::runtime::TupleIdxTable {

Node48::Node48(std::size_t compression_length)
    : Node(NodeType::N48, compression_length) {
  for (int32_t i = 0; i < 256; i++) {
    child_index[i] = Node::EMPTY_MARKER;
  }
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

std::unique_ptr<Node>* Node48::GetChild(int32_t pos) {
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

void Node48::Insert(std::unique_ptr<Node>& node, uint8_t key_byte,
                    std::unique_ptr<Node>& child) {
  Node48* n = static_cast<Node48*>(node.get());

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
    auto new_node = std::make_unique<Node256>(n->prefix_length);
    for (int32_t i = 0; i < 256; i++) {
      if (n->child_index[i] != Node::EMPTY_MARKER) {
        new_node->child[i] = std::move(n->child[n->child_index[i]]);
      }
    }
    new_node->count = n->count;
    CopyPrefix(n, new_node.get());
    node = std::move(new_node);
    Node256::Insert(node, key_byte, child);
  }
}

void Node48::Erase(std::unique_ptr<Node>& node, int pos) {
  Node48* n = static_cast<Node48*>(node.get());

  n->child[n->child_index[pos]].reset();
  n->child_index[pos] = Node::EMPTY_MARKER;
  n->count--;
  if (node->count <= 12) {
    auto new_node = std::make_unique<Node16>(n->prefix_length);
    CopyPrefix(n, new_node.get());
    for (int32_t i = 0; i < 256; i++) {
      if (n->child_index[i] != Node::EMPTY_MARKER) {
        new_node->key[new_node->count] = i;
        new_node->child[new_node->count++] =
            std::move(n->child[n->child_index[i]]);
      }
    }
    node = std::move(new_node);
  }
}

}  // namespace kush::runtime::TupleIdxTable