#include "khir/asm/dominators.h"

#include "gtest/gtest.h"

using namespace kush;
using namespace kush::khir;

TEST(DominatorsTest, SplitGraph) {
  std::vector<std::vector<int>> bb_pred{{}, {0}, {1}, {1}, {2, 3}};

  std::vector<std::vector<int>> dom_tree{{1}, {2, 3, 4}, {}, {}, {}};
  EXPECT_EQ(ComputeDominators(bb_pred), dom_tree);
}

TEST(DominatorsTest, LoopWithMultipleExit) {
  std::vector<std::vector<int>> bb_pred{{}, {0, 3}, {1}, {2}, {1}, {2}};

  std::vector<std::vector<int>> dom_tree{{1}, {2, 4}, {3, 5}, {}, {}, {}};
  EXPECT_EQ(ComputeDominators(bb_pred), dom_tree);
}