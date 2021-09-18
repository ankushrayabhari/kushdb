#include "khir/asm/dfs_label.h"

#include "gtest/gtest.h"

using namespace kush;
using namespace kush::khir;

TEST(DFSLabelTest, ChainGraph) {
  std::vector<std::vector<int>> succ{{1, 5}, {2}, {3}, {}, {}, {}};
  auto result = DFSLabel(succ);

  std::vector<int> expected_preorder{0, 1, 2, 3, -1, 7};
  EXPECT_EQ(result.preorder_label, expected_preorder);

  std::vector<int> expected_postorder{9, 6, 5, 4, -1, 8};
  EXPECT_EQ(result.postorder_label, expected_postorder);
}

TEST(DFSLabelTest, LoopGraph) {
  std::vector<std::vector<int>> succ{{1}, {2, 4}, {3}, {1}, {}};
  auto result = DFSLabel(succ);

  std::vector<int> expected_preorder{0, 1, 2, 3, 6};
  EXPECT_EQ(result.preorder_label, expected_preorder);

  std::vector<int> expected_postorder{9, 8, 5, 4, 7};
  EXPECT_EQ(result.postorder_label, expected_postorder);
}

TEST(DFSLabelTest, LoopWithMultipleExit) {
  std::vector<std::vector<int>> succ{{1}, {2, 4}, {3, 5}, {1}, {}, {}};
  auto result = DFSLabel(succ);

  std::vector<int> expected_preorder{0, 1, 2, 3, 8, 5};
  EXPECT_EQ(result.preorder_label, expected_preorder);

  std::vector<int> expected_postorder{11, 10, 7, 4, 9, 6};
  EXPECT_EQ(result.postorder_label, expected_postorder);
}