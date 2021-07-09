#include "khir/asm/asm_backend.h"

#include <gtest/gtest.h>

#include "khir/program_builder.h"
#include "khir/program_printer.h"

using namespace kush;

TEST(ASMBackendTest, I8_ADD) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I8Type(), {program.I8Type(), program.I8Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.AddI8(args[0], args[1]);
  program.Return(sum);

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(int8_t, int8_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(int8_t(0) + int8_t(0), compute(0, 0));
  EXPECT_EQ(int8_t(-1) + int8_t(1), compute(-1, 1));
  EXPECT_EQ(int8_t(16) + int8_t(0), compute(16, 0));
  EXPECT_EQ(int8_t(-7) + int8_t(-100), compute(-7, -100));
  EXPECT_EQ(int8_t(5) + int8_t(8), compute(5, 8));
}

TEST(ASMBackendTest, I8_SUB) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I8Type(), {program.I8Type(), program.I8Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.SubI8(args[0], args[1]);
  program.Return(sum);

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(int8_t, int8_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(int8_t(0) - int8_t(0), compute(0, 0));
  EXPECT_EQ(int8_t(-1) - int8_t(1), compute(-1, 1));
  EXPECT_EQ(int8_t(16) - int8_t(0), compute(16, 0));
  EXPECT_EQ(int8_t(-7) - int8_t(-100), compute(-7, -100));
  EXPECT_EQ(int8_t(5) - int8_t(8), compute(5, 8));
}

TEST(ASMBackendTest, I8_MUL) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I8Type(), {program.I8Type(), program.I8Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.MulI8(args[0], args[1]);
  program.Return(sum);

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(int8_t, int8_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(int8_t(0) * int8_t(0), compute(0, 0));
  EXPECT_EQ(int8_t(-1) * int8_t(1), compute(-1, 1));
  EXPECT_EQ(int8_t(16) * int8_t(0), compute(16, 0));
  EXPECT_EQ(int8_t(-7) * int8_t(-13), compute(-7, -13));
  EXPECT_EQ(int8_t(5) * int8_t(8), compute(5, 8));
}

TEST(ASMBackendTest, I8_CONST) {
  for (auto x : {-100, 255, 17, 91}) {
    int8_t c = x;

    khir::ProgramBuilder program;
    program.CreatePublicFunction(program.I8Type(), {}, "compute");
    program.Return(program.ConstI8(c));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int8_t()>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    EXPECT_EQ(c, compute());
  }
}

TEST(ASMBackendTest, I8_ZEXT_I64) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(program.I64Type(),
                                           {program.I8Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.I64ZextI8(args[0]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int64_t(int8_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(int64_t(0xEB), compute(0xEB));
  EXPECT_EQ(int64_t(0xA5), compute(0xA5));
  EXPECT_EQ(int64_t(0xC9), compute(0xC9));
  EXPECT_EQ(int64_t(0xFF), compute(0xFF));
  EXPECT_EQ(int64_t(0x70), compute(0x70));
}