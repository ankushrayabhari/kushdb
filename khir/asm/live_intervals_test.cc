#include "khir/asm/live_intervals.h"

#include "gtest/gtest.h"

using namespace kush;
using namespace kush::khir;

TEST(LiveIntervalsTest, SingleBasicBlock) {
  ProgramBuilder program;
  auto type = program.I32Type();
  auto func = program.CreatePublicFunction(
      program.VoidType(), {program.PointerType(type), type}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.StoreI32(args[0], args[1]);
  program.Return();

  auto result =
      ComputeLiveIntervals(program.GetFunction(func), program.GetTypeManager());
}
