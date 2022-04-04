#include "runtime/tuple_idx_table/tuple_idx_table.h"

#include "gtest/gtest.h"

using namespace kush;
using namespace kush::runtime;

TEST(TupleIdxTableTest, EmptyScan) {
  TupleIdxTable::TupleIdxTable table;
  TupleIdxTable::Iterator it;
  table.Begin(it);
  EXPECT_FALSE(table.IteratorNext(it));
}

TEST(TupleIdxTableTest, SingleEntryScan) {
  constexpr int len = 3;
  int tuple_idx[len]{0, 1, 2};

  TupleIdxTable::TupleIdxTable table;
  table.Insert(tuple_idx, len);

  TupleIdxTable::Iterator it;
  EXPECT_TRUE(table.Begin(it));
  EXPECT_EQ(memcmp(tuple_idx, it.node->value->Data(), len * sizeof(int32_t)),
            0);
  EXPECT_FALSE(table.IteratorNext(it));
}

TEST(TupleIdxTableTest, MultipleValuesSortedOrder) {
  constexpr int len = 3;
  int tuple_idx1[len]{1, 0, 2};
  int tuple_idx2[len]{0, 1, 2};
  int tuple_idx3[len]{1, 2, 0};

  TupleIdxTable::TupleIdxTable table;
  table.Insert(tuple_idx1, len);
  table.Insert(tuple_idx2, len);
  table.Insert(tuple_idx3, len);

  TupleIdxTable::Iterator it;
  EXPECT_TRUE(table.Begin(it));
  EXPECT_EQ(memcmp(tuple_idx2, it.node->value->Data(), len * sizeof(int32_t)),
            0);
  EXPECT_TRUE(table.IteratorNext(it));
  EXPECT_EQ(memcmp(tuple_idx1, it.node->value->Data(), len * sizeof(int32_t)),
            0);
  EXPECT_TRUE(table.IteratorNext(it));
  EXPECT_EQ(memcmp(tuple_idx3, it.node->value->Data(), len * sizeof(int32_t)),
            0);
  EXPECT_FALSE(table.IteratorNext(it));
}

TEST(TupleIdxTableTest, MultipleValuesSortedOrderWithDuplicates) {
  constexpr int len = 3;
  int tuple_idx1[len]{1, 0, 2};
  int tuple_idx2[len]{0, 1, 2};
  int tuple_idx3[len]{1, 2, 0};

  TupleIdxTable::TupleIdxTable table;
  table.Insert(tuple_idx1, len);
  table.Insert(tuple_idx2, len);
  table.Insert(tuple_idx3, len);
  table.Insert(tuple_idx3, len);
  table.Insert(tuple_idx2, len);
  table.Insert(tuple_idx1, len);

  TupleIdxTable::Iterator it;
  EXPECT_TRUE(table.Begin(it));
  EXPECT_EQ(memcmp(tuple_idx2, it.node->value->Data(), len * sizeof(int32_t)),
            0);
  EXPECT_TRUE(table.IteratorNext(it));
  EXPECT_EQ(memcmp(tuple_idx1, it.node->value->Data(), len * sizeof(int32_t)),
            0);
  EXPECT_TRUE(table.IteratorNext(it));
  EXPECT_EQ(memcmp(tuple_idx3, it.node->value->Data(), len * sizeof(int32_t)),
            0);
  EXPECT_FALSE(table.IteratorNext(it));
}