#include "runtime/tuple_idx_table/tuple_idx_table.h"

#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

#include "runtime/tuple_idx_table/key.h"
#include "runtime/tuple_idx_table/leaf.h"
#include "runtime/tuple_idx_table/node.h"
#include "runtime/tuple_idx_table/node16.h"
#include "runtime/tuple_idx_table/node256.h"
#include "runtime/tuple_idx_table/node4.h"
#include "runtime/tuple_idx_table/node48.h"

namespace runtime::TupleIdxTable {

void TupleIdxTable::Insert(int32_t* tuple_idx, int32_t len) {
  Insert(tree_, Key::CreateKey(tuple_idx, len), 0);
}

void TupleIdxTable::Insert(std::unique_ptr<Node>& node,
                           std::unique_ptr<Key> value, uint32_t depth) {
  Key& key = *value;
  if (!node) {
    // node is currently empty, create a leaf here with the key
    node = std::make_unique<Leaf>(std::move(value));
    return;
  }

  if (node->type == NodeType::NLeaf) {
    // Replace leaf with Node4 and store both leaves in it
    auto leaf = static_cast<Leaf*>(node.get());

    Key& existing_key = *leaf->value;
    uint32_t new_prefix_length = 0;
    // Leaf node is already there
    if (depth + new_prefix_length == existing_key.Length() &&
        existing_key.Length() == key.Length()) {
      return;
    }

    while (existing_key[depth + new_prefix_length] ==
           key[depth + new_prefix_length]) {
      new_prefix_length++;
      // Leaf node is already there
      if (depth + new_prefix_length == existing_key.Length() &&
          existing_key.Length() == key.Length()) {
        return;
      }
    }

    std::unique_ptr<Node> new_node = std::make_unique<Node4>(new_prefix_length);
    new_node->prefix_length = new_prefix_length;
    memcpy(new_node->prefix.get(), &key[depth], new_prefix_length);
    Node4::Insert(new_node, existing_key[depth + new_prefix_length], node);
    std::unique_ptr<Node> leaf_node = std::make_unique<Leaf>(std::move(value));
    Node4::Insert(new_node, key[depth + new_prefix_length], leaf_node);
    node = std::move(new_node);
    return;
  }

  // Handle prefix of inner node
  if (node->prefix_length) {
    uint32_t mismatch_pos = Node::PrefixMismatch(node.get(), key, depth);
    if (mismatch_pos != node->prefix_length) {
      // Prefix differs, create new node
      std::unique_ptr<Node> new_node = std::make_unique<Node4>(mismatch_pos);
      new_node->prefix_length = mismatch_pos;
      memcpy(new_node->prefix.get(), node->prefix.get(), mismatch_pos);

      // Break up prefix
      auto node_ptr = node.get();
      Node4::Insert(new_node, node->prefix[mismatch_pos], node);
      node_ptr->prefix_length -= (mismatch_pos + 1);
      memmove(node_ptr->prefix.get(), node_ptr->prefix.get() + mismatch_pos + 1,
              node_ptr->prefix_length);

      std::unique_ptr<Node> leaf_node =
          std::make_unique<Leaf>(std::move(value));
      Node4::Insert(new_node, key[depth + mismatch_pos], leaf_node);
      node = std::move(new_node);
      return;
    }

    depth += node->prefix_length;
  }

  // Recurse
  int32_t pos = node->GetChildPos(key[depth]);
  if (pos != -1) {
    auto child = node->GetChild(pos);
    return Insert(*child, std::move(value), depth + 1);
  }
  std::unique_ptr<Node> new_node = std::make_unique<Leaf>(std::move(value));
  Node::InsertLeaf(node, key[depth], new_node);
}

}  // namespace runtime::TupleIdxTable