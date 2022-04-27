#include "runtime/tuple_idx_table/tuple_idx_table.h"

#include <algorithm>
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

namespace kush::runtime::TupleIdxTable {

void InsertImpl(Allocator& allocator, Node*& node, Key* value, uint32_t depth) {
  Key& key = *value;
  if (!node) {
    // node is currently empty, create a leaf here with the key
    node = allocator.Allocate<Leaf>(allocator, value);
    return;
  }

  if (node->type == NodeType::NLeaf) {
    // Replace leaf with Node4 and store both leaves in it
    auto leaf = static_cast<Leaf*>(node);

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

    Node* new_node = allocator.Allocate<Node4>(allocator, new_prefix_length);
    new_node->prefix_length = new_prefix_length;
    memcpy(new_node->prefix, &key[depth], new_prefix_length);
    Node4::Insert(new_node, existing_key[depth + new_prefix_length], node);

    Node* leaf_node = allocator.Allocate<Leaf>(allocator, std::move(value));
    Node4::Insert(new_node, key[depth + new_prefix_length], leaf_node);
    node = std::move(new_node);
    return;
  }

  // Handle prefix of inner node
  if (node->prefix_length) {
    uint32_t mismatch_pos = Node::PrefixMismatch(node, key, depth);
    if (mismatch_pos != node->prefix_length) {
      // Prefix differs, create new node
      Node* new_node = allocator.Allocate<Node4>(allocator, mismatch_pos);
      new_node->prefix_length = mismatch_pos;
      memcpy(new_node->prefix, node->prefix, mismatch_pos);

      // Break up prefix
      auto node_ptr = node;
      Node4::Insert(new_node, node->prefix[mismatch_pos], node);
      node_ptr->prefix_length -= (mismatch_pos + 1);
      memmove(node_ptr->prefix, node_ptr->prefix + mismatch_pos + 1,
              node_ptr->prefix_length);

      Node* leaf_node = allocator.Allocate<Leaf>(allocator, std::move(value));
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
    return InsertImpl(allocator, *child, std::move(value), depth + 1);
  }
  Node* new_node = allocator.Allocate<Leaf>(allocator, std::move(value));
  Node::InsertLeaf(node, key[depth], new_node);
}

void Insert(TupleIdxTable* t, const int32_t* tuple_idx, int32_t len) {
  InsertImpl(t->allocator, t->tree,
             Key::CreateKey(t->allocator, tuple_idx, len), 0);
}

IteratorEntry::IteratorEntry() : node(nullptr), pos(0) {}

IteratorEntry::IteratorEntry(Node* n, int32_t p) : node(n), pos(p) {}

void Iterator::SetEntry(int32_t entry_depth, IteratorEntry entry) {
  if (stack.size() < entry_depth + 1) {
    stack.resize(std::max(
        {(std::size_t)8, (std::size_t)entry_depth + 1, stack.size() * 2}));
  }
  stack[entry_depth] = entry;
}

void BeginImpl(Iterator& it, Node& node) {
  Node* next = nullptr;
  int32_t pos = 0;
  switch (node.type) {
    case NodeType::NLeaf:
      it.node = (Leaf*)&node;
      return;
    case NodeType::N4:
      next = ((Node4&)node).child[0];
      break;
    case NodeType::N16:
      next = ((Node16&)node).child[0];
      break;
    case NodeType::N48: {
      auto& n48 = (Node48&)node;
      while (n48.child_index[pos] == Node::EMPTY_MARKER) {
        pos++;
      }
      next = n48.child[n48.child_index[pos]];
      break;
    }
    case NodeType::N256: {
      auto& n256 = (Node256&)node;
      while (!n256.child[pos]) {
        pos++;
      }
      next = n256.child[pos];
      break;
    }
  }
  it.SetEntry(it.depth, IteratorEntry(&node, pos));
  it.depth++;
  return BeginImpl(it, *next);
}

bool Begin(TupleIdxTable* table, Iterator* it) {
  if (table->tree) {
    BeginImpl(*it, *table->tree);
    return true;
  }
  return false;
}

bool IteratorNext(TupleIdxTable* table, Iterator* it) {
  // Skip leaf
  if ((it->depth) &&
      ((it->stack[it->depth - 1].node)->type == NodeType::NLeaf)) {
    it->depth--;
  }

  // Look for the next leaf
  while (it->depth > 0) {
    auto& top = it->stack[it->depth - 1];
    Node* node = top.node;

    if (node->type == NodeType::NLeaf) {
      // found a leaf: move to next node
      it->node = (Leaf*)node;
      return true;
    }

    // Find next node
    top.pos = node->GetNextPos(top.pos);
    if (top.pos != -1) {
      // next node found: go there
      it->SetEntry(it->depth, IteratorEntry(*node->GetChild(top.pos), -1));
      it->depth++;
    } else {
      // no node found: move up the tree
      it->depth--;
    }
  }

  return false;
}

TupleIdxTable* Create() { return new TupleIdxTable; }

void Free(TupleIdxTable* t) { delete t; }

Iterator* CreateIt() { return new Iterator; }

void FreeIt(Iterator* t) { delete t; }

int32_t* Get(Iterator* t) { return t->node->value->Data(); }

}  // namespace kush::runtime::TupleIdxTable