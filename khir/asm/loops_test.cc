#include "khir/asm/loops.h"

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

TEST(LoopsTest, NoLoop) {
  std::vector<std::vector<int>> bb_succ{{1}, {2, 3}, {4}, {4}, {}};

  std::vector<std::vector<int>> loop_tree{{}, {}, {}, {}, {}};
  EXPECT_EQ(FindLoops(GenerateBlocks(bb_succ)), loop_tree);
}

TEST(LoopsTest, SimpleLoop) {
  std::vector<std::vector<int>> bb_succ{{1}, {2, 3}, {1}, {}};

  std::vector<std::vector<int>> loop_tree{{}, {2}, {}, {}};
  EXPECT_EQ(FindLoops(GenerateBlocks(bb_succ)), loop_tree);
}

TEST(LoopsTest, SimpleLoopWithEarlyReturn) {
  std::vector<std::vector<int>> bb_succ{{1}, {2}, {3, 4}, {}, {1}};

  std::vector<std::vector<int>> loop_tree{{}, {2, 4}, {}, {}, {}};
  EXPECT_EQ(FindLoops(GenerateBlocks(bb_succ)), loop_tree);
}

TEST(LoopsTest, NestedLoop) {
  std::vector<std::vector<int>> bb_succ{{1}, {2, 5}, {3, 4}, {2}, {1}, {}};

  std::vector<std::vector<int>> loop_tree{{}, {2, 4}, {3}, {}, {}, {}};
  EXPECT_EQ(FindLoops(GenerateBlocks(bb_succ)), loop_tree);
}