#include "runtime/tuple_idx_table/node.h"

#include <cstdint>
#include <cstring>
#include <memory>

#include "runtime/tuple_idx_table/allocator.h"
#include "runtime/tuple_idx_table/key.h"
#include "runtime/tuple_idx_table/node16.h"
#include "runtime/tuple_idx_table/node256.h"
#include "runtime/tuple_idx_table/node4.h"
#include "runtime/tuple_idx_table/node48.h"

namespace kush::runtime::TupleIdxTable {

Node::Node(Allocator& a, NodeType type, std::size_t compressed_prefix_size)
    : prefix_length(0),
      count(0),
      type(type),
      prefix(a.AllocateData(compressed_prefix_size)),
      allocator(a) {}

void Node::CopyPrefix(Node* src, Node* dst) {
  dst->prefix_length = src->prefix_length;
  memcpy(dst->prefix, src->prefix, src->prefix_length);
}

Node** Node::GetChild(int32_t pos) { return nullptr; }

int32_t Node::GetMin() { return 0; }

uint32_t Node::PrefixMismatch(Node* node, Key& key, uint64_t depth) {
  uint32_t pos;
  for (pos = 0; pos < node->prefix_length; pos++) {
    if (key[depth + pos] != node->prefix[pos]) {
      return pos;
    }
  }
  return pos;
}

void Node::InsertLeaf(Node*& node, uint8_t key, Node*& new_node) {
  switch (node->type) {
    case NodeType::N4:
      Node4::Insert(node, key, new_node);
      break;
    case NodeType::N16:
      Node16::Insert(node, key, new_node);
      break;
    case NodeType::N48:
      Node48::Insert(node, key, new_node);
      break;
    case NodeType::N256:
      Node256::Insert(node, key, new_node);
      break;
    default:
      throw std::runtime_error("Unrecognized leaf type for insert");
  }
}

}  // namespace kush::runtime::TupleIdxTable