#include "khir/asm/live_intervals.h"

#include "gtest/gtest.h"

#include "khir/program_builder.h"

using namespace kush;
using namespace kush::khir;

TEST(LiveIntervalsTest, SingleBasicBlock) {
  ProgramBuilder program;

  auto type = program.I32Type();
  auto ex = program.DeclareExternalFunction("test", program.PointerType(type),
                                            {}, nullptr);

  program.CreatePublicFunction(type, {}, "compute");
  auto x_arr = program.Call(ex);
  auto x1 = program.LoadI32(program.ConstGEP(type, x_arr, {0}));
  auto x2 = program.LoadI32(program.ConstGEP(type, x_arr, {1}));
  program.Return(program.AddI32(x1, x2));

  auto built = program.Build();
  std::vector<bool> gep_materialize(built.Functions()[1].Instrs().size(),
                                    false);
  auto res = ComputeLiveIntervals(built.Functions()[1], gep_materialize,
                                  built.TypeManager());

  // TODO: add expectations
}
