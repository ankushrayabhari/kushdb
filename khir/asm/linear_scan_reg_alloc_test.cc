#include "khir/asm/linear_scan_reg_alloc.h"

#include <gtest/gtest.h>

using namespace kush;
using namespace kush::khir;

TEST(LiveIntervalsTest, StoreInstructionForcedIntoRegister) {
  for (auto type_func : {&ProgramBuilder::I8Type, &ProgramBuilder::I16Type,
                         &ProgramBuilder::I32Type, &ProgramBuilder::I64Type,
                         &ProgramBuilder::F64Type}) {
    ProgramBuilder program;
    auto type = std::invoke(type_func, program);
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.PointerType(type), type}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreI32(args[0], args[1]);
    program.Return();

    auto result = LinearScanRegisterAlloc(program.GetFunction(func),
                                          program.GetTypeManager());
    EXPECT_EQ(result.assignment.size(), 4);
    EXPECT_TRUE(result.assignment[2].IsRegister());
  }
}

TEST(LiveIntervalsTest, FlagRegIntoBranch) {
  ProgramBuilder program;
  auto func = program.CreatePublicFunction(program.I32Type(),
                                           {program.I1Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto bb1 = program.GenerateBlock();
  auto bb2 = program.GenerateBlock();
  program.Branch(args[0], bb1, bb2);
  program.SetCurrentBlock(bb1);
  program.Return(program.ConstI32(5));
  program.SetCurrentBlock(bb2);
  program.Return(program.ConstI32(6));

  auto result = LinearScanRegisterAlloc(program.GetFunction(func),
                                        program.GetTypeManager());
  EXPECT_TRUE(result.assignment[0].IsRegister());
  EXPECT_EQ(result.assignment[0].Register(), 100);
}