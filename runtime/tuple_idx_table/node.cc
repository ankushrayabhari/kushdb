#include "runtime/tuple_idx_table/node.h"

#include <cstdint>
#include <cstring>
#include <memory>

#include "runtime/tuple_idx_table/key.h"
#include "runtime/tuple_idx_table/node16.h"
#include "runtime/tuple_idx_table/node256.h"
#include "runtime/tuple_idx_table/node4.h"
#include "runtime/tuple_idx_table/node48.h"

namespace runtime::TupleIdxTable {

Node::Node(NodeType type, std::size_t compressed_prefix_size)
    : prefix_length(0),
      count(0),
      type(type),
      prefix(new uint8_t[compressed_prefix_size]) {}

void Node::CopyPrefix(Node* src, Node* dst) {
  dst->prefix_length = src->prefix_length;
  memcpy(dst->prefix.get(), src->prefix.get(), src->prefix_length);
}

std::unique_ptr<Node>* Node::GetChild(int32_t pos) { return nullptr; }

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

void Node::InsertLeaf(std::unique_ptr<Node>& node, uint8_t key,
                      std::unique_ptr<Node>& new_node) {
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

void Node::Erase(std::unique_ptr<Node>& node, int32_t pos) {
  switch (node->type) {
    case NodeType::N4: {
      Node4::Erase(node, pos);
      break;
    }
    case NodeType::N16: {
      Node16::Erase(node, pos);
      break;
    }
    case NodeType::N48: {
      Node48::Erase(node, pos);
      break;
    }
    case NodeType::N256:
      Node256::Erase(node, pos);
      break;
    default:
      throw std::runtime_error("Unrecognized leaf type for erase");
  }
}

}  // namespace runtime::TupleIdxTable