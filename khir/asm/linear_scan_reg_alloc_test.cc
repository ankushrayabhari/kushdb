#include "khir/asm/linear_scan_reg_alloc.h"

#include "gtest/gtest.h"

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

    auto rpo = RPOLabel(program.GetFunction(func).BasicBlockSuccessors());
    auto result = LinearScanRegisterAlloc(program.GetFunction(func),
                                          program.GetTypeManager(), rpo);
    EXPECT_EQ(result.size(), 4);
    EXPECT_TRUE(result[2].IsRegister());
  }
}

TEST(LiveIntervalsTest, I8FlagRegIntoBranch) {
  for (auto cmp_type : {CompType::EQ, CompType::NE, CompType::LT, CompType::LE,
                        CompType::GT, CompType::GE}) {
    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I32Type(),
                                             {program.I8Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto bb1 = program.GenerateBlock();
    auto bb2 = program.GenerateBlock();
    auto cond = program.CmpI8(cmp_type, args[0], program.ConstI8(1));
    program.Branch(cond, bb1, bb2);
    program.SetCurrentBlock(bb1);
    program.Return(program.ConstI32(5));
    program.SetCurrentBlock(bb2);
    program.Return(program.ConstI32(6));

    auto rpo = RPOLabel(program.GetFunction(func).BasicBlockSuccessors());
    auto result = LinearScanRegisterAlloc(program.GetFunction(func),
                                          program.GetTypeManager(), rpo);
    EXPECT_TRUE(result[cond.GetIdx()].IsRegister());
    EXPECT_EQ(result[cond.GetIdx()].Register(), 100);
  }
}

TEST(LiveIntervalsTest, I16FlagRegIntoBranch) {
  for (auto cmp_type : {CompType::EQ, CompType::NE, CompType::LT, CompType::LE,
                        CompType::GT, CompType::GE}) {
    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I32Type(),
                                             {program.I16Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto bb1 = program.GenerateBlock();
    auto bb2 = program.GenerateBlock();
    auto cond = program.CmpI16(cmp_type, args[0], program.ConstI16(1));
    program.Branch(cond, bb1, bb2);
    program.SetCurrentBlock(bb1);
    program.Return(program.ConstI32(5));
    program.SetCurrentBlock(bb2);
    program.Return(program.ConstI32(6));

    auto rpo = RPOLabel(program.GetFunction(func).BasicBlockSuccessors());
    auto result = LinearScanRegisterAlloc(program.GetFunction(func),
                                          program.GetTypeManager(), rpo);
    EXPECT_TRUE(result[cond.GetIdx()].IsRegister());
    EXPECT_EQ(result[cond.GetIdx()].Register(), 100);
  }
}

TEST(LiveIntervalsTest, I32FlagRegIntoBranch) {
  for (auto cmp_type : {CompType::EQ, CompType::NE, CompType::LT, CompType::LE,
                        CompType::GT, CompType::GE}) {
    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I32Type(),
                                             {program.I32Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto bb1 = program.GenerateBlock();
    auto bb2 = program.GenerateBlock();
    auto cond = program.CmpI32(cmp_type, args[0], program.ConstI32(1));
    program.Branch(cond, bb1, bb2);
    program.SetCurrentBlock(bb1);
    program.Return(program.ConstI32(5));
    program.SetCurrentBlock(bb2);
    program.Return(program.ConstI32(6));

    auto rpo = RPOLabel(program.GetFunction(func).BasicBlockSuccessors());
    auto result = LinearScanRegisterAlloc(program.GetFunction(func),
                                          program.GetTypeManager(), rpo);
    EXPECT_TRUE(result[cond.GetIdx()].IsRegister());
    EXPECT_EQ(result[cond.GetIdx()].Register(), 100);
  }
}

TEST(LiveIntervalsTest, I64FlagRegIntoBranch) {
  for (auto cmp_type : {CompType::EQ, CompType::NE, CompType::LT, CompType::LE,
                        CompType::GT, CompType::GE}) {
    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I32Type(),
                                             {program.I64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto bb1 = program.GenerateBlock();
    auto bb2 = program.GenerateBlock();
    auto cond = program.CmpI64(cmp_type, args[0], program.ConstI64(1));
    program.Branch(cond, bb1, bb2);
    program.SetCurrentBlock(bb1);
    program.Return(program.ConstI32(5));
    program.SetCurrentBlock(bb2);
    program.Return(program.ConstI32(6));

    auto rpo = RPOLabel(program.GetFunction(func).BasicBlockSuccessors());
    auto result = LinearScanRegisterAlloc(program.GetFunction(func),
                                          program.GetTypeManager(), rpo);
    EXPECT_TRUE(result[cond.GetIdx()].IsRegister());
    EXPECT_EQ(result[cond.GetIdx()].Register(), 100);
  }
}

TEST(LiveIntervalsTest, F64FlagRegIntoBranch) {
  for (auto cmp_type : {CompType::EQ, CompType::NE, CompType::LT, CompType::LE,
                        CompType::GT, CompType::GE}) {
    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I32Type(),
                                             {program.F64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto bb1 = program.GenerateBlock();
    auto bb2 = program.GenerateBlock();
    auto cond = program.CmpF64(cmp_type, args[0], program.ConstF64(1));
    program.Branch(cond, bb1, bb2);
    program.SetCurrentBlock(bb1);
    program.Return(program.ConstI32(5));
    program.SetCurrentBlock(bb2);
    program.Return(program.ConstI32(6));

    auto rpo = RPOLabel(program.GetFunction(func).BasicBlockSuccessors());
    auto result = LinearScanRegisterAlloc(program.GetFunction(func),
                                          program.GetTypeManager(), rpo);
    EXPECT_TRUE(result[cond.GetIdx()].IsRegister());
    EXPECT_EQ(result[cond.GetIdx()].Register(), 101);
  }
}