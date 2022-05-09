#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "runtime/allocator.h"
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
  Leaf* node = nullptr;
  int32_t depth = 0;
  std::vector<IteratorEntry> stack;

  void SetEntry(int32_t depth, IteratorEntry entry);
};

struct TupleIdxTable {
  Allocator allocator;
  Node* tree = nullptr;
};

bool Insert(TupleIdxTable* t, const int32_t* tuple_idx, int32_t len);

bool Begin(TupleIdxTable* t, Iterator* it);

bool IteratorNext(TupleIdxTable* t, Iterator* it);

TupleIdxTable* Create();
void Free(TupleIdxTable* t);

Iterator* CreateIt();
void FreeIt(Iterator* t);
int32_t* Get(Iterator* t);

}  // namespace kush::runtime::TupleIdxTable