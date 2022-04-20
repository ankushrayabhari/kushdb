#include "khir/asm/dfs_label.h"

#include "gtest/gtest.h"

using namespace kush;
using namespace kush::khir;

std::vector<BasicBlock> GenerateBlocks(std::vector<std::vector<int>>& succ) {
  std::vector<std::vector<int>> pred(succ.size());
  for (int i = 0; i < succ.size(); i++) {
    for (int j : succ[i]) {
      pred[j].push_back(i);
    }
  }

  std::vector<BasicBlock> output;
  for (int i = 0; i < succ.size(); i++) {
    output.push_back(BasicBlock({}, succ[i], pred[i]));
  }
  return output;
}

TEST(DFSLabelTest, ChainGraph) {
  std::vector<std::vector<int>> succ{{1, 5}, {2}, {3}, {}, {}, {}};
  auto result = DFSLabel(GenerateBlocks(succ));

  std::vector<int> expected_preorder{0, 1, 2, 3, -1, 7};
  EXPECT_EQ(result.preorder_label, expected_preorder);

  std::vector<int> expected_postorder{9, 6, 5, 4, -1, 8};
  EXPECT_EQ(result.postorder_label, expected_postorder);
}

TEST(DFSLabelTest, LoopGraph) {
  std::vector<std::vector<int>> succ{{1}, {2, 4}, {3}, {1}, {}};
  auto result = DFSLabel(GenerateBlocks(succ));

  std::vector<int> expected_preorder{0, 1, 2, 3, 6};
  EXPECT_EQ(result.preorder_label, expected_preorder);

  std::vector<int> expected_postorder{9, 8, 5, 4, 7};
  EXPECT_EQ(result.postorder_label, expected_postorder);
}

TEST(DFSLabelTest, LoopWithMultipleExit) {
  std::vector<std::vector<int>> succ{{1}, {2, 4}, {3, 5}, {1}, {}, {}};
  auto result = DFSLabel(GenerateBlocks(succ));

  std::vector<int> expected_preorder{0, 1, 2, 3, 8, 5};
  EXPECT_EQ(result.preorder_label, expected_preorder);

  std::vector<int> expected_postorder{11, 10, 7, 4, 9, 6};
  EXPECT_EQ(result.postorder_label, expected_postorder);
}