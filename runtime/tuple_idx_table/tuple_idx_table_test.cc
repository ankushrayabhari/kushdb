#include "runtime/tuple_idx_table/tuple_idx_table.h"

#include "gtest/gtest.h"

using namespace kush;
using namespace kush::runtime;

TEST(TupleIdxTableTest, EmptyScan) {
  TupleIdxTable::TupleIdxTable table;
  TupleIdxTable::Iterator it;
  EXPECT_FALSE(Begin(&table, &it));
  EXPECT_FALSE(IteratorNext(&table, &it));
}

TEST(TupleIdxTableTest, SingleEntryScan) {
  constexpr int len = 3;
  int tuple_idx[len]{0, 1, 2};

  TupleIdxTable::TupleIdxTable table;
  EXPECT_EQ(Insert(&table, tuple_idx, len), true);

  TupleIdxTable::Iterator it;
  EXPECT_TRUE(Begin(&table, &it));
  EXPECT_EQ(memcmp(tuple_idx, Get(&it), len * sizeof(int32_t)), 0);
  EXPECT_FALSE(IteratorNext(&table, &it));
}

TEST(TupleIdxTableTest, MultipleValuesSortedOrder) {
  constexpr int len = 3;
  int tuple_idx1[len]{1, 0, 2};
  int tuple_idx2[len]{0, 1, 2};
  int tuple_idx3[len]{1, 2, 0};

  TupleIdxTable::TupleIdxTable table;
  EXPECT_EQ(Insert(&table, tuple_idx1, len), true);
  EXPECT_EQ(Insert(&table, tuple_idx2, len), true);
  EXPECT_EQ(Insert(&table, tuple_idx3, len), true);

  TupleIdxTable::Iterator it;
  EXPECT_TRUE(Begin(&table, &it));
  EXPECT_EQ(memcmp(tuple_idx2, Get(&it), len * sizeof(int32_t)), 0);
  EXPECT_TRUE(IteratorNext(&table, &it));
  EXPECT_EQ(memcmp(tuple_idx1, Get(&it), len * sizeof(int32_t)), 0);
  EXPECT_TRUE(IteratorNext(&table, &it));
  EXPECT_EQ(memcmp(tuple_idx3, Get(&it), len * sizeof(int32_t)), 0);
  EXPECT_FALSE(IteratorNext(&table, &it));
}

TEST(TupleIdxTableTest, MultipleValuesSortedOrderWithDuplicates) {
  constexpr int len = 3;
  int tuple_idx1[len]{1, 0, 2};
  int tuple_idx2[len]{0, 1, 2};
  int tuple_idx3[len]{1, 2, 0};

  TupleIdxTable::TupleIdxTable table;
  EXPECT_EQ(Insert(&table, tuple_idx1, len), true);
  EXPECT_EQ(Insert(&table, tuple_idx2, len), true);
  EXPECT_EQ(Insert(&table, tuple_idx3, len), true);
  EXPECT_EQ(Insert(&table, tuple_idx3, len), false);
  EXPECT_EQ(Insert(&table, tuple_idx2, len), false);
  EXPECT_EQ(Insert(&table, tuple_idx1, len), false);

  TupleIdxTable::Iterator it;
  EXPECT_TRUE(Begin(&table, &it));
  EXPECT_EQ(memcmp(tuple_idx2, Get(&it), len * sizeof(int32_t)), 0);
  EXPECT_TRUE(IteratorNext(&table, &it));
  EXPECT_EQ(memcmp(tuple_idx1, Get(&it), len * sizeof(int32_t)), 0);
  EXPECT_TRUE(IteratorNext(&table, &it));
  EXPECT_EQ(memcmp(tuple_idx3, Get(&it), len * sizeof(int32_t)), 0);
  EXPECT_FALSE(IteratorNext(&table, &it));
}