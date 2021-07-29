#include "khir/asm/live_intervals.h"

#include <gtest/gtest.h>

using namespace kush;
using namespace kush::khir;

TEST(LiveIntervalsTest, SingleBasicBlockArgs) {
  ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I32Type(), {program.I32Type(), program.I32Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.AddI32(args[0], args[1]);
  program.Return(sum);

  auto [live_intervals, order] =
      ComputeLiveIntervals(program.GetFunction(func), program.GetTypeManager());

  std::sort(live_intervals.begin(), live_intervals.end(),
            [](const LiveInterval& l1, const LiveInterval& l2) {
              return l1.Value().GetIdx() < l2.Value().GetIdx();
            });

  EXPECT_EQ(live_intervals.size(), 3);

  EXPECT_EQ(live_intervals[0].Value(), args[0]);
  EXPECT_EQ(live_intervals[0].StartBB(), order[0]);
  EXPECT_EQ(live_intervals[0].EndBB(), order[0]);
  EXPECT_EQ(live_intervals[0].StartIdx(), 0);
  EXPECT_EQ(live_intervals[0].EndIdx(), 2);

  EXPECT_EQ(live_intervals[1].Value(), args[1]);
  EXPECT_EQ(live_intervals[1].StartBB(), order[0]);
  EXPECT_EQ(live_intervals[1].EndBB(), order[0]);
  EXPECT_EQ(live_intervals[1].StartIdx(), 1);
  EXPECT_EQ(live_intervals[1].EndIdx(), 2);

  EXPECT_EQ(live_intervals[2].Value(), sum);
  EXPECT_EQ(live_intervals[2].StartBB(), order[0]);
  EXPECT_EQ(live_intervals[2].EndBB(), order[0]);
  EXPECT_EQ(live_intervals[2].StartIdx(), 2);
  EXPECT_EQ(live_intervals[2].EndIdx(), 3);
}