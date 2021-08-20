#include "khir/asm/live_intervals.h"

#include "gtest/gtest.h"

using namespace kush;
using namespace kush::khir;

TEST(LiveIntervalsTest, SingleBasicBlockArgs) {
  ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I32Type(), {program.I32Type(), program.I32Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.AddI32(args[0], args[1]);
  program.Return(sum);

  auto analysis =
      ComputeLiveIntervals(program.GetFunction(func), program.GetTypeManager());
  auto& live_intervals = analysis.live_intervals;
  auto& labels = analysis.labels;

  std::sort(live_intervals.begin(), live_intervals.end(),
            [](const LiveInterval& l1, const LiveInterval& l2) {
              return l1.Value().GetIdx() < l2.Value().GetIdx();
            });

  EXPECT_EQ(live_intervals.size(), 3);

  EXPECT_EQ(live_intervals[0].Value(), args[0]);
  EXPECT_EQ(live_intervals[0].StartBB(), labels[0]);
  EXPECT_EQ(live_intervals[0].StartIdx(), 0);
  EXPECT_EQ(live_intervals[0].EndBB(), labels[0]);
  EXPECT_EQ(live_intervals[0].EndIdx(), 2);

  EXPECT_EQ(live_intervals[1].Value(), args[1]);
  EXPECT_EQ(live_intervals[1].StartBB(), labels[0]);
  EXPECT_EQ(live_intervals[1].StartIdx(), 1);
  EXPECT_EQ(live_intervals[1].EndBB(), labels[0]);
  EXPECT_EQ(live_intervals[1].EndIdx(), 2);

  EXPECT_EQ(live_intervals[2].Value(), sum);
  EXPECT_EQ(live_intervals[2].StartBB(), labels[0]);
  EXPECT_EQ(live_intervals[2].StartIdx(), 2);
  EXPECT_EQ(live_intervals[2].EndBB(), labels[0]);
  EXPECT_EQ(live_intervals[2].EndIdx(), 3);
}

TEST(LiveIntervalsTest, Store) {
  ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I32Type(),
      {program.PointerType(program.I32Type()), program.I32Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.StoreI32(args[0], args[1]);
  program.Return();

  auto analysis =
      ComputeLiveIntervals(program.GetFunction(func), program.GetTypeManager());
  auto& live_intervals = analysis.live_intervals;
  auto& labels = analysis.labels;

  std::sort(live_intervals.begin(), live_intervals.end(),
            [](const LiveInterval& l1, const LiveInterval& l2) {
              return l1.Value().GetIdx() < l2.Value().GetIdx();
            });

  EXPECT_EQ(live_intervals.size(), 3);

  EXPECT_EQ(live_intervals[0].Value(), args[0]);
  EXPECT_EQ(live_intervals[0].StartBB(), labels[0]);
  EXPECT_EQ(live_intervals[0].StartIdx(), 0);
  EXPECT_EQ(live_intervals[0].EndBB(), labels[0]);
  EXPECT_EQ(live_intervals[0].EndIdx(), 2);

  EXPECT_EQ(live_intervals[1].Value(), args[1]);
  EXPECT_EQ(live_intervals[1].StartBB(), labels[0]);
  EXPECT_EQ(live_intervals[1].StartIdx(), 1);
  EXPECT_EQ(live_intervals[1].EndBB(), labels[0]);
  EXPECT_EQ(live_intervals[1].EndIdx(), 2);

  EXPECT_EQ(live_intervals[2].Value(), khir::Value(2));
  EXPECT_EQ(live_intervals[2].StartBB(), labels[0]);
  EXPECT_EQ(live_intervals[2].StartIdx(), 2);
  EXPECT_EQ(live_intervals[2].EndBB(), labels[0]);
  EXPECT_EQ(live_intervals[2].EndIdx(), 2);
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

  auto analysis =
      ComputeLiveIntervals(program.GetFunction(func), program.GetTypeManager());
  auto& live_intervals = analysis.live_intervals;
  auto& labels = analysis.labels;

  std::sort(live_intervals.begin(), live_intervals.end(),
            [](const LiveInterval& l1, const LiveInterval& l2) {
              return l1.Value().GetIdx() < l2.Value().GetIdx();
            });

  EXPECT_EQ(live_intervals.size(), 3);

  EXPECT_EQ(live_intervals[0].Value(), args[0]);
  EXPECT_EQ(live_intervals[0].StartBB(), labels[0]);
  EXPECT_EQ(live_intervals[0].StartIdx(), 0);
  EXPECT_EQ(live_intervals[0].EndBB(), labels[0]);
  EXPECT_EQ(live_intervals[0].EndIdx(), 2);

  EXPECT_EQ(live_intervals[1].Value(), args[1]);
  EXPECT_EQ(live_intervals[1].StartBB(), labels[0]);
  EXPECT_EQ(live_intervals[1].StartIdx(), 1);
  EXPECT_EQ(live_intervals[1].EndBB(), labels[0]);
  EXPECT_EQ(live_intervals[1].EndIdx(), 2);

  EXPECT_EQ(live_intervals[2].Value(), sum);
  EXPECT_EQ(live_intervals[2].StartBB(), labels[0]);
  EXPECT_EQ(live_intervals[2].StartIdx(), 2);
  EXPECT_EQ(live_intervals[2].EndBB(), labels[1]);
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

  auto analysis =
      ComputeLiveIntervals(program.GetFunction(func), program.GetTypeManager());
  auto& live_intervals = analysis.live_intervals;
  auto& labels = analysis.labels;

  std::sort(live_intervals.begin(), live_intervals.end(),
            [](const LiveInterval& l1, const LiveInterval& l2) {
              return l1.Value().GetIdx() < l2.Value().GetIdx();
            });

  EXPECT_EQ(live_intervals.size(), 4);

  EXPECT_EQ(live_intervals[0].Value(), args[0]);
  EXPECT_EQ(live_intervals[0].StartBB(), labels[0]);
  EXPECT_EQ(live_intervals[0].StartIdx(), 0);
  EXPECT_EQ(live_intervals[0].EndBB(), labels[0]);
  EXPECT_EQ(live_intervals[0].EndIdx(), 3);

  EXPECT_EQ(live_intervals[1].Value(), args[1]);
  EXPECT_EQ(live_intervals[1].StartBB(), labels[0]);
  EXPECT_EQ(live_intervals[1].StartIdx(), 1);
  EXPECT_EQ(live_intervals[1].EndBB(), labels[1]);
  EXPECT_EQ(live_intervals[1].EndIdx(), 4);

  EXPECT_EQ(live_intervals[2].Value(), args[2]);
  EXPECT_EQ(live_intervals[2].StartBB(), labels[0]);
  EXPECT_EQ(live_intervals[2].StartIdx(), 2);
  EXPECT_EQ(live_intervals[2].EndBB(), labels[2]);
  EXPECT_EQ(live_intervals[2].EndIdx(), 6);

  EXPECT_EQ(live_intervals[3].Value(), phi);
  if (labels[1] < labels[2]) {
    EXPECT_EQ(live_intervals[3].StartBB(), labels[1]);
    EXPECT_EQ(live_intervals[3].StartIdx(), 4);
  } else {
    EXPECT_EQ(live_intervals[3].StartBB(), labels[2]);
    EXPECT_EQ(live_intervals[3].StartIdx(), 6);
  }
  EXPECT_EQ(live_intervals[3].EndBB(), labels[3]);
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

  auto analysis =
      ComputeLiveIntervals(program.GetFunction(func), program.GetTypeManager());
  auto& live_intervals = analysis.live_intervals;
  auto& labels = analysis.labels;

  std::sort(live_intervals.begin(), live_intervals.end(),
            [](const LiveInterval& l1, const LiveInterval& l2) {
              return l1.Value().GetIdx() < l2.Value().GetIdx();
            });

  EXPECT_EQ(live_intervals.size(), 4);

  EXPECT_EQ(live_intervals[0].Value(), phi);
  EXPECT_EQ(live_intervals[0].StartBB(), labels[0]);
  EXPECT_EQ(live_intervals[0].StartIdx(), 0);
  EXPECT_EQ(live_intervals[0].EndBB(), labels[3]);
  EXPECT_EQ(live_intervals[0].EndIdx(), 8);

  EXPECT_EQ(live_intervals[1].Value(), cond);
  EXPECT_EQ(live_intervals[1].StartBB(), labels[1]);
  EXPECT_EQ(live_intervals[1].StartIdx(), 3);
  EXPECT_EQ(live_intervals[1].EndBB(), labels[1]);
  EXPECT_EQ(live_intervals[1].EndIdx(), 4);

  EXPECT_EQ(live_intervals[2].Value(), incr);
  EXPECT_EQ(live_intervals[2].StartBB(), labels[2]);
  EXPECT_EQ(live_intervals[2].StartIdx(), 5);
  EXPECT_EQ(live_intervals[2].EndBB(), labels[2]);
  EXPECT_EQ(live_intervals[2].EndIdx(), 6);

  EXPECT_EQ(live_intervals[3].Value(), scaled);
  EXPECT_EQ(live_intervals[3].StartBB(), labels[3]);
  EXPECT_EQ(live_intervals[3].StartIdx(), 8);
  EXPECT_EQ(live_intervals[3].EndBB(), labels[3]);
  EXPECT_EQ(live_intervals[3].EndIdx(), 9);
}

TEST(LiveIntervalsTest, NestedLoop) {
  ProgramBuilder program;
  auto type = program.I32Type();
  auto func = program.CreatePublicFunction(type, {}, "compute");

  auto init = program.ConstI32(0);
  auto phi_1 = program.PhiMember(init);

  auto bb1 = program.GenerateBlock();
  auto bb2 = program.GenerateBlock();
  auto bb3 = program.GenerateBlock();
  auto bb4 = program.GenerateBlock();
  auto bb5 = program.GenerateBlock();
  auto bb6 = program.GenerateBlock();

  program.Branch(bb1);

  program.SetCurrentBlock(bb1);
  auto phi = program.Phi(type);
  program.UpdatePhiMember(phi, phi_1);
  auto cond = program.CmpI32(CompType::LT, phi, program.ConstI32(10));
  program.Branch(cond, bb2, bb3);

  program.SetCurrentBlock(bb2);
  auto test = program.AddI32(phi, program.ConstI32(1));
  auto nested_init = program.ConstI32(0);
  auto nested_phi_1 = program.PhiMember(nested_init);
  program.Branch(bb4);

  program.SetCurrentBlock(bb4);
  auto nested_phi = program.Phi(type);
  program.UpdatePhiMember(nested_phi, nested_phi_1);
  auto nested_cond =
      program.CmpI32(CompType::LT, nested_phi, program.ConstI32(10));
  program.Branch(nested_cond, bb5, bb6);

  program.SetCurrentBlock(bb5);
  auto nested_incr = program.AddI32(test, nested_phi);
  auto nested_phi_2 = program.PhiMember(nested_incr);
  program.UpdatePhiMember(nested_phi, nested_phi_2);
  program.Branch(bb4);

  program.SetCurrentBlock(bb6);
  auto incr = program.AddI32(phi, program.ConstI32(1));
  auto phi_2 = program.PhiMember(incr);
  program.UpdatePhiMember(phi, phi_2);
  program.Branch(bb1);

  program.SetCurrentBlock(bb3);
  auto scaled = program.MulI32(phi, program.ConstI32(3));
  program.Return(scaled);

  auto analysis =
      ComputeLiveIntervals(program.GetFunction(func), program.GetTypeManager());
  auto& live_intervals = analysis.live_intervals;
  auto& labels = analysis.labels;

  std::sort(live_intervals.begin(), live_intervals.end(),
            [](const LiveInterval& l1, const LiveInterval& l2) {
              return l1.Value().GetIdx() < l2.Value().GetIdx();
            });

  EXPECT_EQ(live_intervals[2].Value(), test);
  EXPECT_EQ(live_intervals[2].StartBB(), labels[2]);
  EXPECT_EQ(live_intervals[2].StartIdx(), 5);
  EXPECT_EQ(live_intervals[2].EndBB(), labels[5]);
  EXPECT_EQ(live_intervals[2].EndIdx(), 13);
}