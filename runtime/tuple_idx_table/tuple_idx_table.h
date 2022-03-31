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

namespace runtime::TupleIdxTable {

class TupleIdxTable final {
 public:
  TupleIdxTable();

  void Insert(int32_t* tuple_idx, int32_t len);

 private:
  void Insert(std::unique_ptr<Node>& node, std::unique_ptr<Key> value,
              uint32_t depth);

  std::unique_ptr<Node> tree_;
};

}  // namespace runtime::TupleIdxTable