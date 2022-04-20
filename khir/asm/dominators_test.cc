#include "khir/asm/dominators.h"

#include "gtest/gtest.h"

using namespace kush;
using namespace kush::khir;

std::vector<std::vector<int>> PredFromSucc(
    const std::vector<std::vector<int>>& bb_succ) {
  std::vector<std::vector<int>> bb_pred(bb_succ.size());
  for (int from = 0; from < bb_pred.size(); from++) {
    for (int to : bb_succ[from]) {
      bb_pred[to].push_back(from);
    }
  }
  return bb_pred;
}

std::vector<BasicBlock> GenerateBlocks(std::vector<std::vector<int>>& bb_succ,
                                       std::vector<std::vector<int>>& bb_pred) {
  std::vector<BasicBlock> output;
  for (int i = 0; i < bb_succ.size(); i++) {
    output.push_back(BasicBlock({}, bb_succ[i], bb_pred[i]));
  }
  return output;
}

TEST(DominatorsTest, SplitGraph) {
  std::vector<std::vector<int>> bb_succ{{1}, {2, 3}, {4}, {4}, {}};
  auto bb_pred = PredFromSucc(bb_succ);

  std::vector<std::vector<int>> dom_tree{{1}, {2, 3, 4}, {}, {}, {}};
  EXPECT_EQ(ComputeDominatorTree(GenerateBlocks(bb_succ, bb_pred)), dom_tree);
}

TEST(DominatorsTest, LoopWithMultipleExit) {
  std::vector<std::vector<int>> bb_succ{{1}, {2, 4}, {3, 5}, {1}, {}, {}, {}};
  auto bb_pred = PredFromSucc(bb_succ);

  std::vector<std::vector<int>> dom_tree{{1}, {2, 4}, {3, 5}, {}, {}, {}, {}};
  EXPECT_EQ(ComputeDominatorTree(GenerateBlocks(bb_succ, bb_pred)), dom_tree);
}