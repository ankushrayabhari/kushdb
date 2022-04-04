#pragma once

#include <cstdint>
#include <memory>

#include "runtime/tuple_idx_table/key.h"

namespace kush::runtime::TupleIdxTable {

enum class NodeType : uint8_t { N4 = 0, N16 = 1, N48 = 2, N256 = 3, NLeaf = 4 };

class Node {
 public:
  static const uint8_t EMPTY_MARKER = 48;

  Node(NodeType type, std::size_t compressed_prefix_size);
  virtual ~Node() {}

  // Get the position of a child corresponding exactly to the specific byte,
  // returns -1 if not exists
  virtual int32_t GetChildPos(uint8_t k) { return -1; }

  // Get the position of the biggest element in node
  virtual int32_t GetMin();

  // Get the next position in the node, or -1 if there
  // is no next position. if pos == -1, then the first
  // valid position in the node will be returned.
  virtual int32_t GetNextPos(int32_t pos) { return -1; }

  // Get the child at the specified position in the node. pos should be between
  // [0, count). Throws an assertion if the element is not found.
  virtual std::unique_ptr<Node>* GetChild(int32_t pos);

  // Compare the key with the prefix of the node, return the number matching
  // bytes
  static uint32_t PrefixMismatch(Node* node, Key& key, uint64_t depth);

  // Insert leaf into inner node
  static void InsertLeaf(std::unique_ptr<Node>& node, uint8_t key,
                         std::unique_ptr<Node>& new_node);

  // Erase entry from node
  static void Erase(std::unique_ptr<Node>& node, int32_t pos);

  // Get the position of the first child that is greater or equal to the
  // specific byte, or -1 if there are no children
  // matching the criteria
  virtual int32_t GetChildGreaterEqual(uint8_t k, bool& equal) {
    throw std::runtime_error("Unimplemented GetChildGreaterEqual for Node");
  }

 protected:
  // Copies the prefix from the source to the destination node
  static void CopyPrefix(Node* src, Node* dst);

 public:
  // length of the compressed path (prefix)
  uint32_t prefix_length;

  // number of non-null children
  uint16_t count;

  // node type
  NodeType type;

  // compressed path (prefix)
  std::unique_ptr<uint8_t[]> prefix;
};

}  // namespace kush::runtime::TupleIdxTable