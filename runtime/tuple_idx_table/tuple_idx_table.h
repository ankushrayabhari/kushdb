#pragma once

#include <cstdint>
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

struct IteratorEntry {
  IteratorEntry();
  IteratorEntry(Node* node, int32_t pos);

  Node* node;
  int32_t pos;
};

struct Iterator {
  //! The current Leaf Node, valid if depth>0
  Leaf* node = nullptr;

  //! The current depth
  int32_t depth = 0;

  //! Stack, the size is determined at runtime
  std::vector<IteratorEntry> stack;

  bool start = false;

  void SetEntry(int32_t depth, IteratorEntry entry);
};

class TupleIdxTable final {
 public:
  void Insert(int32_t* tuple_idx, int32_t len);

  // Scan
  bool Begin(Iterator& it);
  bool IteratorNext(Iterator& it);

 private:
  void BeginImpl(Iterator& it, Node& node);
  void Insert(std::unique_ptr<Node>& node, std::unique_ptr<Key> value,
              uint32_t depth);

  std::unique_ptr<Node> tree_;
};

}  // namespace kush::runtime::TupleIdxTable