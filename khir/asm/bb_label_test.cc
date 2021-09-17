#include "khir/asm/bb_label.h"

#include "gtest/gtest.h"

using namespace kush;
using namespace kush::khir;

TEST(BBLabelTest, ChainGraph) {
  std::vector<std::vector<int>> bb_succ{{1, 5}, {2}, {3}, {}, {}, {}};
  auto result = BBLabel(bb_succ);

  std::vector<int> expected_preorder{0, 1, 2, 3, -1, 7};
  EXPECT_EQ(result.preorder_label, expected_preorder);

  std::vector<int> expected_postorder{9, 6, 5, 4, -1, 8};
  EXPECT_EQ(result.postorder_label, expected_postorder);
}

TEST(BBLabelTest, LoopGraph) {
  std::vector<std::vector<int>> bb_succ{{1}, {2, 4}, {3}, {1}, {}};
  auto result = BBLabel(bb_succ);

  std::vector<int> expected_preorder{0, 1, 2, 3, 6};
  EXPECT_EQ(result.preorder_label, expected_preorder);

  std::vector<int> expected_postorder{9, 8, 5, 4, 7};
  EXPECT_EQ(result.postorder_label, expected_postorder);
}

TEST(BBLabelTest, LoopWithMultipleExit) {
  std::vector<std::vector<int>> bb_succ{{1}, {2, 4}, {3, 5}, {1}, {}, {}};
  auto result = BBLabel(bb_succ);

  std::vector<int> expected_preorder{0, 1, 2, 3, 8, 5};
  EXPECT_EQ(result.preorder_label, expected_preorder);

  std::vector<int> expected_postorder{11, 10, 7, 4, 9, 6};
  EXPECT_EQ(result.postorder_label, expected_postorder);
}