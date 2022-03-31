#include "runtime/tuple_idx_table/node4.h"

#include <cstring>
#include <memory>

#include "runtime/tuple_idx_table/node16.h"

namespace runtime::TupleIdxTable {

Node4::Node4(std::size_t compression_length)
    : Node(NodeType::N4, compression_length) {
  memset(key, 0, sizeof(key));
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

std::unique_ptr<Node>* Node4::GetChild(int32_t pos) {
  if (pos >= count) {
    throw std::runtime_error("invalid pos");
  }

  return &child[pos];
}

void Node4::Insert(std::unique_ptr<Node>& node, uint8_t key_byte,
                   std::unique_ptr<Node>& child) {
  Node4* n = static_cast<Node4*>(node.get());

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
    auto new_node = std::make_unique<Node16>(n->prefix_length);
    new_node->count = 4;
    CopyPrefix(node.get(), new_node.get());
    for (int32_t i = 0; i < 4; i++) {
      new_node->key[i] = n->key[i];
      new_node->child[i] = std::move(n->child[i]);
    }
    node = std::move(new_node);
    Node16::Insert(node, key_byte, child);
  }
}

void Node4::Erase(std::unique_ptr<Node>& node, int pos) {
  Node4* n = static_cast<Node4*>(node.get());
  if (pos >= n->count) {
    throw std::runtime_error("invalid pos");
  }

  // erase the child and decrease the count
  n->child[pos].reset();
  n->count--;

  // potentially move any children backwards
  for (; pos < n->count; pos++) {
    n->key[pos] = n->key[pos + 1];
    n->child[pos] = std::move(n->child[pos + 1]);
  }

  // This is a one way node
  if (n->count == 1) {
    auto childref = n->child[0].get();
    //! concatenate prefixes
    auto new_length = node->prefix_length + childref->prefix_length + 1;
    //! have to allocate space in our prefix array
    std::unique_ptr<uint8_t[]> new_prefix(new uint8_t[new_length]);

    //! first move the existing prefix (if any)
    for (uint32_t i = 0; i < childref->prefix_length; i++) {
      new_prefix[new_length - (i + 1)] =
          childref->prefix[childref->prefix_length - (i + 1)];
    }
    //! now move the current key as part of the prefix
    new_prefix[node->prefix_length] = n->key[0];
    //! finally add the old prefix
    for (uint32_t i = 0; i < node->prefix_length; i++) {
      new_prefix[i] = node->prefix[i];
    }
    //! set new prefix and move the child
    childref->prefix = std::move(new_prefix);
    childref->prefix_length = new_length;
    node = std::move(n->child[0]);
  }
}

}  // namespace runtime::TupleIdxTable