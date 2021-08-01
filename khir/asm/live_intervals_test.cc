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
  EXPECT_EQ(live_intervals[0].StartIdx(), 0);
  EXPECT_EQ(live_intervals[0].EndBB(), order[0]);
  EXPECT_EQ(live_intervals[0].EndIdx(), 2);

  EXPECT_EQ(live_intervals[1].Value(), args[1]);
  EXPECT_EQ(live_intervals[1].StartBB(), order[0]);
  EXPECT_EQ(live_intervals[1].StartIdx(), 1);
  EXPECT_EQ(live_intervals[1].EndBB(), order[0]);
  EXPECT_EQ(live_intervals[1].EndIdx(), 2);

  EXPECT_EQ(live_intervals[2].Value(), sum);
  EXPECT_EQ(live_intervals[2].StartBB(), order[0]);
  EXPECT_EQ(live_intervals[2].StartIdx(), 2);
  EXPECT_EQ(live_intervals[2].EndBB(), order[0]);
  EXPECT_EQ(live_intervals[2].EndIdx(), 3);
}

TEST(LiveIntervalsTest, AcrossBasicBlock) {
  ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I32Type(), {program.I32Type(), program.I32Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.AddI32(args[0], args[1]);
  auto bb = program.GenerateBlock();
  program.Branch(bb);
  program.SetCurrentBlock(bb);
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
  EXPECT_EQ(live_intervals[0].StartIdx(), 0);
  EXPECT_EQ(live_intervals[0].EndBB(), order[0]);
  EXPECT_EQ(live_intervals[0].EndIdx(), 2);

  EXPECT_EQ(live_intervals[1].Value(), args[1]);
  EXPECT_EQ(live_intervals[1].StartBB(), order[0]);
  EXPECT_EQ(live_intervals[1].StartIdx(), 1);
  EXPECT_EQ(live_intervals[1].EndBB(), order[0]);
  EXPECT_EQ(live_intervals[1].EndIdx(), 2);

  EXPECT_EQ(live_intervals[2].Value(), sum);
  EXPECT_EQ(live_intervals[2].StartBB(), order[0]);
  EXPECT_EQ(live_intervals[2].StartIdx(), 2);
  EXPECT_EQ(live_intervals[2].EndBB(), order[1]);
  EXPECT_EQ(live_intervals[2].EndIdx(), 4);
}

TEST(LiveIntervalsTest, Phi) {
  ProgramBuilder program;
  auto type = program.I32Type();
  auto func = program.CreatePublicFunction(type, {program.I1Type(), type, type},
                                           "compute");
  auto args = program.GetFunctionArguments(func);

  auto bb1 = program.GenerateBlock();
  auto bb2 = program.GenerateBlock();
  auto bb3 = program.GenerateBlock();

  program.Branch(args[0], bb1, bb2);

  program.SetCurrentBlock(bb1);
  auto phi_1 = program.PhiMember(args[1]);
  program.Branch(bb3);

  program.SetCurrentBlock(bb2);
  auto phi_2 = program.PhiMember(args[2]);
  program.Branch(bb3);

  program.SetCurrentBlock(bb3);
  auto phi = program.Phi(type);
  program.UpdatePhiMember(phi, phi_1);
  program.UpdatePhiMember(phi, phi_2);
  program.Return(phi);

  auto [live_intervals, order] =
      ComputeLiveIntervals(program.GetFunction(func), program.GetTypeManager());

  std::sort(live_intervals.begin(), live_intervals.end(),
            [](const LiveInterval& l1, const LiveInterval& l2) {
              return l1.Value().GetIdx() < l2.Value().GetIdx();
            });

  EXPECT_EQ(live_intervals.size(), 4);

  EXPECT_EQ(live_intervals[0].Value(), args[0]);
  EXPECT_EQ(live_intervals[0].StartBB(), order[0]);
  EXPECT_EQ(live_intervals[0].StartIdx(), 0);
  EXPECT_EQ(live_intervals[0].EndBB(), order[0]);
  EXPECT_EQ(live_intervals[0].EndIdx(), 3);

  EXPECT_EQ(live_intervals[1].Value(), args[1]);
  EXPECT_EQ(live_intervals[1].StartBB(), order[0]);
  EXPECT_EQ(live_intervals[1].StartIdx(), 1);
  EXPECT_EQ(live_intervals[1].EndBB(), order[1]);
  EXPECT_EQ(live_intervals[1].EndIdx(), 4);

  EXPECT_EQ(live_intervals[2].Value(), args[2]);
  EXPECT_EQ(live_intervals[2].StartBB(), order[0]);
  EXPECT_EQ(live_intervals[2].StartIdx(), 2);
  EXPECT_EQ(live_intervals[2].EndBB(), order[2]);
  EXPECT_EQ(live_intervals[2].EndIdx(), 6);

  EXPECT_EQ(live_intervals[3].Value(), phi);
  if (order[1] < order[2]) {
    EXPECT_EQ(live_intervals[3].StartBB(), order[1]);
    EXPECT_EQ(live_intervals[3].StartIdx(), 4);
  } else {
    EXPECT_EQ(live_intervals[3].StartBB(), order[2]);
    EXPECT_EQ(live_intervals[3].StartIdx(), 6);
  }
  EXPECT_EQ(live_intervals[3].EndBB(), order[3]);
  EXPECT_EQ(live_intervals[3].EndIdx(), 9);
}

TEST(LiveIntervalsTest, Loop) {
  ProgramBuilder program;
  auto type = program.I32Type();
  auto func = program.CreatePublicFunction(type, {}, "compute");

  auto init = program.ConstI32(0);
  auto phi_1 = program.PhiMember(init);

  auto bb1 = program.GenerateBlock();
  auto bb2 = program.GenerateBlock();
  auto bb3 = program.GenerateBlock();

  program.Branch(bb1);

  program.SetCurrentBlock(bb1);
  auto phi = program.Phi(type);
  program.UpdatePhiMember(phi, phi_1);

  auto cond = program.CmpI32(CompType::LT, phi, program.ConstI32(10));
  program.Branch(cond, bb2, bb3);

  program.SetCurrentBlock(bb2);
  auto incr = program.AddI32(phi, program.ConstI32(1));
  auto phi_2 = program.PhiMember(incr);
  program.UpdatePhiMember(phi, phi_2);
  program.Branch(bb1);

  program.SetCurrentBlock(bb3);
  auto scaled = program.MulI32(phi, program.ConstI32(3));
  program.Return(scaled);

  auto [live_intervals, order] =
      ComputeLiveIntervals(program.GetFunction(func), program.GetTypeManager());

  std::sort(live_intervals.begin(), live_intervals.end(),
            [](const LiveInterval& l1, const LiveInterval& l2) {
              return l1.Value().GetIdx() < l2.Value().GetIdx();
            });

  EXPECT_EQ(live_intervals.size(), 4);

  EXPECT_EQ(live_intervals[0].Value(), phi);
  EXPECT_EQ(live_intervals[0].StartBB(), order[0]);
  EXPECT_EQ(live_intervals[0].StartIdx(), 0);
  EXPECT_EQ(live_intervals[0].EndBB(), order[3]);
  EXPECT_EQ(live_intervals[0].EndIdx(), 8);

  EXPECT_EQ(live_intervals[1].Value(), cond);
  EXPECT_EQ(live_intervals[1].StartBB(), order[1]);
  EXPECT_EQ(live_intervals[1].StartIdx(), 3);
  EXPECT_EQ(live_intervals[1].EndBB(), order[1]);
  EXPECT_EQ(live_intervals[1].EndIdx(), 4);

  EXPECT_EQ(live_intervals[2].Value(), incr);
  EXPECT_EQ(live_intervals[2].StartBB(), order[2]);
  EXPECT_EQ(live_intervals[2].StartIdx(), 5);
  EXPECT_EQ(live_intervals[2].EndBB(), order[2]);
  EXPECT_EQ(live_intervals[2].EndIdx(), 6);

  EXPECT_EQ(live_intervals[3].Value(), scaled);
  EXPECT_EQ(live_intervals[3].StartBB(), order[3]);
  EXPECT_EQ(live_intervals[3].StartIdx(), 8);
  EXPECT_EQ(live_intervals[3].EndBB(), order[3]);
  EXPECT_EQ(live_intervals[3].EndIdx(), 9);
}