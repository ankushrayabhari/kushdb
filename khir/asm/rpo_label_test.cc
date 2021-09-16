#include "khir/asm/rpo_label.h"

#include "gtest/gtest.h"

using namespace kush;
using namespace kush::khir;

TEST(RPOLabelTest, ChainGraph) {
  std::vector<std::vector<int>> bb_succ{{1, 5}, {2}, {3}, {}, {}, {}};
  auto result = RPOLabel(bb_succ);

  std::vector<int> expected_order{0, 5, 1, 2, 3};
  EXPECT_EQ(result.order, expected_order);

  std::vector<int> expected_label{0, 2, 3, 4, -1, 1};
  EXPECT_EQ(result.label_per_block, expected_label);
}

TEST(RPOLabelTest, LoopGraph) {
  std::vector<std::vector<int>> bb_succ{{1}, {2, 4}, {3}, {1}, {}};
  auto result = RPOLabel(bb_succ);

  std::vector<int> expected_order{0, 1, 4, 2, 3};
  EXPECT_EQ(result.order, expected_order);

  std::vector<int> expected_label{0, 1, 3, 4, 2};
  EXPECT_EQ(result.label_per_block, expected_label);
}

TEST(RPOLabelTest, LoopWithMultipleExit) {
  std::vector<std::vector<int>> bb_succ{{1}, {2, 4}, {3, 5}, {1}, {}, {}};
  auto result = RPOLabel(bb_succ);

  std::vector<int> expected_order{0, 1, 4, 2, 5, 3};
  EXPECT_EQ(result.order, expected_order);

  std::vector<int> expected_label{0, 1, 3, 5, 2, 4};
  EXPECT_EQ(result.label_per_block, expected_label);
}