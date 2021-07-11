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

TEST(ASMBackendTest, I8_CONV_F64) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(program.F64Type(),
                                           {program.I8Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.F64ConvI8(args[0]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<double(int8_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(0.0, compute(0.0));
  EXPECT_EQ(127.0, compute(127));
  EXPECT_EQ(-128.0, compute(-128));
  EXPECT_EQ(15.0, compute(15));
  EXPECT_EQ(-1.0, compute(-1));
}

TEST(ASMBackendTest, I8_CMP_EQ_Return) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.I8Type(), program.I8Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.CmpI8(khir::CompType::EQ, args[0], args[1]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(int8_t, int8_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_NE(0, compute(0, 0));
  EXPECT_EQ(0, compute(0, 1));
  EXPECT_EQ(0, compute(1, 0));
  EXPECT_NE(0, compute(1, 1));
  EXPECT_NE(0, compute(-1, -1));
  EXPECT_EQ(0, compute(-1, 0));
}

TEST(ASMBackendTest, I8_CMP_NE_Return) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.I8Type(), program.I8Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.CmpI8(khir::CompType::NE, args[0], args[1]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(int8_t, int8_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(0, compute(0, 0));
  EXPECT_NE(0, compute(0, 1));
  EXPECT_NE(0, compute(1, 0));
  EXPECT_EQ(0, compute(1, 1));
  EXPECT_EQ(0, compute(-1, -1));
  EXPECT_NE(0, compute(-1, 0));
}

TEST(ASMBackendTest, I8_CMP_LT_Return) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.I8Type(), program.I8Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.CmpI8(khir::CompType::LT, args[0], args[1]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(int8_t, int8_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(0, compute(0, 0));
  EXPECT_NE(0, compute(0, 1));
  EXPECT_EQ(0, compute(1, 0));
  EXPECT_EQ(0, compute(1, 1));
  EXPECT_EQ(0, compute(-1, -1));
  EXPECT_NE(0, compute(-1, 0));
}

TEST(ASMBackendTest, I8_CMP_GT_Return) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.I8Type(), program.I8Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.CmpI8(khir::CompType::GT, args[0], args[1]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(int8_t, int8_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(0, compute(0, 0));
  EXPECT_EQ(0, compute(0, 1));
  EXPECT_NE(0, compute(1, 0));
  EXPECT_EQ(0, compute(1, 1));
  EXPECT_EQ(0, compute(-1, -1));
  EXPECT_EQ(0, compute(-1, 0));
}

TEST(ASMBackendTest, I8_CMP_LE_Return) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.I8Type(), program.I8Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.CmpI8(khir::CompType::LE, args[0], args[1]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(int8_t, int8_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_NE(0, compute(0, 0));
  EXPECT_NE(0, compute(0, 1));
  EXPECT_EQ(0, compute(1, 0));
  EXPECT_NE(0, compute(1, 1));
  EXPECT_NE(0, compute(-1, -1));
  EXPECT_NE(0, compute(-1, 0));
}

TEST(ASMBackendTest, I8_CMP_GE_Return) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.I8Type(), program.I8Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.CmpI8(khir::CompType::GE, args[0], args[1]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(int8_t, int8_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_NE(0, compute(0, 0));
  EXPECT_EQ(0, compute(0, 1));
  EXPECT_NE(0, compute(1, 0));
  EXPECT_NE(0, compute(1, 1));
  EXPECT_NE(0, compute(-1, -1));
  EXPECT_EQ(0, compute(-1, 0));
}

TEST(ASMBackendTest, I8_LOAD) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I8Type(), {program.PointerType(program.I8Type())}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.LoadI8(args[0]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(int8_t*)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  int8_t loc;
  for (int i = -10; i <= 10; i++) {
    loc = i;
    EXPECT_EQ(loc, compute(&loc));
  }
}

TEST(ASMBackendTest, I8_STORE) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.VoidType(),
      {program.PointerType(program.I8Type()), program.I8Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.StoreI8(args[0], args[1]);
  program.Return();

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<void(int8_t*, int8_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  int8_t loc;
  for (int i = -10; i <= 10; i++) {
    compute(&loc, i);
    EXPECT_EQ(loc, i);
  }
}

TEST(ASMBackendTest, I16_ADD) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I16Type(), {program.I16Type(), program.I16Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.AddI16(args[0], args[1]);
  program.Return(sum);

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int16_t(int16_t, int16_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(int16_t(0) + int16_t(0), compute(0, 0));
  EXPECT_EQ(int16_t(-1) + int16_t(1), compute(-1, 1));
  EXPECT_EQ(int16_t(16) + int16_t(0), compute(16, 0));
  EXPECT_EQ(int16_t(-70) + int16_t(-1000), compute(-70, -1000));
  EXPECT_EQ(int16_t(5) + int16_t(8), compute(5, 8));
}

TEST(ASMBackendTest, I16_SUB) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I16Type(), {program.I16Type(), program.I16Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.SubI16(args[0], args[1]);
  program.Return(sum);

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int16_t(int16_t, int16_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(int16_t(0) - int16_t(0), compute(0, 0));
  EXPECT_EQ(int16_t(-1) - int16_t(1), compute(-1, 1));
  EXPECT_EQ(int16_t(16) - int16_t(0), compute(16, 0));
  EXPECT_EQ(int16_t(-70) - int16_t(-1000), compute(-70, -1000));
  EXPECT_EQ(int16_t(5) - int16_t(8), compute(5, 8));
}

TEST(ASMBackendTest, I16_MUL) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I16Type(), {program.I16Type(), program.I16Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.MulI16(args[0], args[1]);
  program.Return(sum);

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int16_t(int16_t, int16_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(int16_t(0) * int16_t(0), compute(0, 0));
  EXPECT_EQ(int16_t(-1) * int16_t(1), compute(-1, 1));
  EXPECT_EQ(int16_t(16) * int16_t(0), compute(16, 0));
  EXPECT_EQ(int16_t(-7) * int16_t(-13), compute(-7, -13));
  EXPECT_EQ(int16_t(5) * int16_t(8), compute(5, 8));
}

TEST(ASMBackendTest, I16_CONST) {
  for (auto x : {-100, 255, 17, 91}) {
    int16_t c = x;

    khir::ProgramBuilder program;
    program.CreatePublicFunction(program.I16Type(), {}, "compute");
    program.Return(program.ConstI16(c));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int16_t()>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    EXPECT_EQ(c, compute());
  }
}

TEST(ASMBackendTest, I16_ZEXT_I64) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(program.I64Type(),
                                           {program.I16Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.I64ZextI16(args[0]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int64_t(int16_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(int64_t(0xFFEB), compute(0xFFEB));
  EXPECT_EQ(int64_t(0xFFA5), compute(0xFFA5));
  EXPECT_EQ(int64_t(0xFFC9), compute(0xFFC9));
  EXPECT_EQ(int64_t(0xFFFF), compute(0xFFFF));
  EXPECT_EQ(int64_t(0xFF70), compute(0xFF70));
}

TEST(ASMBackendTest, I16_CONV_F64) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(program.F64Type(),
                                           {program.I16Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.F64ConvI16(args[0]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<double(int16_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(0.0, compute(0.0));
  EXPECT_EQ(127.0, compute(127));
  EXPECT_EQ(-255.0, compute(-255));
  EXPECT_EQ(15.0, compute(15));
  EXPECT_EQ(-1.0, compute(-1));
}

TEST(ASMBackendTest, I16_CMP_EQ_Return) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.I16Type(), program.I16Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.CmpI16(khir::CompType::EQ, args[0], args[1]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(int16_t, int16_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_NE(0, compute(0, 0));
  EXPECT_EQ(0, compute(0, 1));
  EXPECT_EQ(0, compute(1, 0));
  EXPECT_NE(0, compute(1, 1));
  EXPECT_NE(0, compute(-1, -1));
  EXPECT_EQ(0, compute(-1, 0));
}

TEST(ASMBackendTest, I16_CMP_NE_Return) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.I16Type(), program.I16Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.CmpI16(khir::CompType::NE, args[0], args[1]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(int16_t, int16_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(0, compute(0, 0));
  EXPECT_NE(0, compute(0, 1));
  EXPECT_NE(0, compute(1, 0));
  EXPECT_EQ(0, compute(1, 1));
  EXPECT_EQ(0, compute(-1, -1));
  EXPECT_NE(0, compute(-1, 0));
}

TEST(ASMBackendTest, I16_CMP_LT_Return) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.I16Type(), program.I16Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.CmpI16(khir::CompType::LT, args[0], args[1]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(int16_t, int16_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(0, compute(0, 0));
  EXPECT_NE(0, compute(0, 1));
  EXPECT_EQ(0, compute(1, 0));
  EXPECT_EQ(0, compute(1, 1));
  EXPECT_EQ(0, compute(-1, -1));
  EXPECT_NE(0, compute(-1, 0));
}

TEST(ASMBackendTest, I16_CMP_GT_Return) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.I16Type(), program.I16Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.CmpI16(khir::CompType::GT, args[0], args[1]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(int16_t, int16_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(0, compute(0, 0));
  EXPECT_EQ(0, compute(0, 1));
  EXPECT_NE(0, compute(1, 0));
  EXPECT_EQ(0, compute(1, 1));
  EXPECT_EQ(0, compute(-1, -1));
  EXPECT_EQ(0, compute(-1, 0));
}

TEST(ASMBackendTest, I16_CMP_LE_Return) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.I16Type(), program.I16Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.CmpI16(khir::CompType::LE, args[0], args[1]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(int16_t, int16_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_NE(0, compute(0, 0));
  EXPECT_NE(0, compute(0, 1));
  EXPECT_EQ(0, compute(1, 0));
  EXPECT_NE(0, compute(1, 1));
  EXPECT_NE(0, compute(-1, -1));
  EXPECT_NE(0, compute(-1, 0));
}

TEST(ASMBackendTest, I16_CMP_GE_Return) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.I16Type(), program.I16Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.CmpI16(khir::CompType::GE, args[0], args[1]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(int16_t, int16_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_NE(0, compute(0, 0));
  EXPECT_EQ(0, compute(0, 1));
  EXPECT_NE(0, compute(1, 0));
  EXPECT_NE(0, compute(1, 1));
  EXPECT_NE(0, compute(-1, -1));
  EXPECT_EQ(0, compute(-1, 0));
}

TEST(ASMBackendTest, I16_LOAD) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I16Type(), {program.PointerType(program.I16Type())}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.LoadI16(args[0]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int16_t(int16_t*)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  int16_t loc;
  for (int i = -10; i <= 10; i++) {
    loc = 2 * i;
    EXPECT_EQ(loc, compute(&loc));
  }
}

TEST(ASMBackendTest, I16_STORE) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.VoidType(),
      {program.PointerType(program.I16Type()), program.I16Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.StoreI16(args[0], args[1]);
  program.Return();

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<void(int16_t*, int16_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  int16_t loc;
  for (int i = -10; i <= 10; i++) {
    compute(&loc, 2 * i);
    EXPECT_EQ(loc, 2 * i);
  }
}

TEST(ASMBackendTest, I32_ADD) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I32Type(), {program.I32Type(), program.I32Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.AddI32(args[0], args[1]);
  program.Return(sum);

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int32_t(int32_t, int32_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(0 + 0, compute(0, 0));
  EXPECT_EQ(-1 + 1, compute(-1, 1));
  EXPECT_EQ(16 + 0, compute(16, 0));
  EXPECT_EQ(-70 + -1000, compute(-70, -1000));
  EXPECT_EQ(5 + 8, compute(5, 8));
}

TEST(ASMBackendTest, I32_SUB) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I32Type(), {program.I32Type(), program.I32Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.SubI32(args[0], args[1]);
  program.Return(sum);

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int32_t(int32_t, int32_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(0 - 0, compute(0, 0));
  EXPECT_EQ(-1 - 1, compute(-1, 1));
  EXPECT_EQ(16 - 0, compute(16, 0));
  EXPECT_EQ(-70 - -1000, compute(-70, -1000));
  EXPECT_EQ(5 - 8, compute(5, 8));
}

TEST(ASMBackendTest, I32_MUL) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I32Type(), {program.I32Type(), program.I32Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.MulI32(args[0], args[1]);
  program.Return(sum);

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int32_t(int32_t, int32_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(0 * 0, compute(0, 0));
  EXPECT_EQ(-1 * 1, compute(-1, 1));
  EXPECT_EQ(16 * 0, compute(16, 0));
  EXPECT_EQ(-7 * -13, compute(-7, -13));
  EXPECT_EQ(5 * 8, compute(5, 8));
}

TEST(ASMBackendTest, I32_CONST) {
  for (auto x : {-100, 255, 17, 91}) {
    int32_t c = x;

    khir::ProgramBuilder program;
    program.CreatePublicFunction(program.I32Type(), {}, "compute");
    program.Return(program.ConstI32(c));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int32_t()>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    EXPECT_EQ(c, compute());
  }
}

TEST(ASMBackendTest, I32_ZEXT_I64) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(program.I64Type(),
                                           {program.I32Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.I64ZextI32(args[0]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int64_t(int32_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(int64_t(0xFFEB), compute(0xFFEB));
  EXPECT_EQ(int64_t(0xFFA5), compute(0xFFA5));
  EXPECT_EQ(int64_t(0xFFC9), compute(0xFFC9));
  EXPECT_EQ(int64_t(0xFFFF), compute(0xFFFF));
  EXPECT_EQ(int64_t(0xFF70), compute(0xFF70));
}

TEST(ASMBackendTest, I32_CONV_F64) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(program.F64Type(),
                                           {program.I32Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.F64ConvI32(args[0]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<double(int32_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(0.0, compute(0.0));
  EXPECT_EQ(127.0, compute(127));
  EXPECT_EQ(-255.0, compute(-255));
  EXPECT_EQ(15.0, compute(15));
  EXPECT_EQ(-1.0, compute(-1));
}

TEST(ASMBackendTest, I32_CMP_EQ_Return) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.I32Type(), program.I32Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.CmpI32(khir::CompType::EQ, args[0], args[1]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(int32_t, int32_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_NE(0, compute(0, 0));
  EXPECT_EQ(0, compute(0, 1));
  EXPECT_EQ(0, compute(1, 0));
  EXPECT_NE(0, compute(1, 1));
  EXPECT_NE(0, compute(-1, -1));
  EXPECT_EQ(0, compute(-1, 0));
}

TEST(ASMBackendTest, I32_CMP_NE_Return) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.I32Type(), program.I32Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.CmpI32(khir::CompType::NE, args[0], args[1]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(int32_t, int32_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(0, compute(0, 0));
  EXPECT_NE(0, compute(0, 1));
  EXPECT_NE(0, compute(1, 0));
  EXPECT_EQ(0, compute(1, 1));
  EXPECT_EQ(0, compute(-1, -1));
  EXPECT_NE(0, compute(-1, 0));
}

TEST(ASMBackendTest, I32_CMP_LT_Return) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.I32Type(), program.I32Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.CmpI32(khir::CompType::LT, args[0], args[1]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(int32_t, int32_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(0, compute(0, 0));
  EXPECT_NE(0, compute(0, 1));
  EXPECT_EQ(0, compute(1, 0));
  EXPECT_EQ(0, compute(1, 1));
  EXPECT_EQ(0, compute(-1, -1));
  EXPECT_NE(0, compute(-1, 0));
}

TEST(ASMBackendTest, I32_CMP_GT_Return) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.I32Type(), program.I32Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.CmpI32(khir::CompType::GT, args[0], args[1]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(int32_t, int32_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(0, compute(0, 0));
  EXPECT_EQ(0, compute(0, 1));
  EXPECT_NE(0, compute(1, 0));
  EXPECT_EQ(0, compute(1, 1));
  EXPECT_EQ(0, compute(-1, -1));
  EXPECT_EQ(0, compute(-1, 0));
}

TEST(ASMBackendTest, I32_CMP_LE_Return) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.I32Type(), program.I32Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.CmpI32(khir::CompType::LE, args[0], args[1]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(int32_t, int32_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_NE(0, compute(0, 0));
  EXPECT_NE(0, compute(0, 1));
  EXPECT_EQ(0, compute(1, 0));
  EXPECT_NE(0, compute(1, 1));
  EXPECT_NE(0, compute(-1, -1));
  EXPECT_NE(0, compute(-1, 0));
}

TEST(ASMBackendTest, I32_CMP_GE_Return) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.I32Type(), program.I32Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.CmpI32(khir::CompType::GE, args[0], args[1]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(int32_t, int32_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_NE(0, compute(0, 0));
  EXPECT_EQ(0, compute(0, 1));
  EXPECT_NE(0, compute(1, 0));
  EXPECT_NE(0, compute(1, 1));
  EXPECT_NE(0, compute(-1, -1));
  EXPECT_EQ(0, compute(-1, 0));
}

TEST(ASMBackendTest, I32_LOAD) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I32Type(), {program.PointerType(program.I32Type())}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.LoadI32(args[0]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int32_t(int32_t*)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  int32_t loc;
  for (int i = -10; i <= 10; i++) {
    loc = 2 * i;
    EXPECT_EQ(loc, compute(&loc));
  }
}

TEST(ASMBackendTest, I32_STORE) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.VoidType(),
      {program.PointerType(program.I32Type()), program.I32Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.StoreI32(args[0], args[1]);
  program.Return();

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<void(int32_t*, int32_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  int32_t loc;
  for (int i = -10; i <= 10; i++) {
    compute(&loc, 2 * i);
    EXPECT_EQ(loc, 2 * i);
  }
}

TEST(ASMBackendTest, I64_ADD) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I64Type(), {program.I64Type(), program.I64Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.AddI64(args[0], args[1]);
  program.Return(sum);

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int64_t(int64_t, int64_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(0 + 0, compute(0, 0));
  EXPECT_EQ(-1 + 1, compute(-1, 1));
  EXPECT_EQ(16 + 0, compute(16, 0));
  EXPECT_EQ(-70 + -1000, compute(-70, -1000));
  EXPECT_EQ(5 + 8, compute(5, 8));
}

TEST(ASMBackendTest, I64_SUB) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I64Type(), {program.I64Type(), program.I64Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.SubI64(args[0], args[1]);
  program.Return(sum);

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int64_t(int64_t, int64_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(0 - 0, compute(0, 0));
  EXPECT_EQ(-1 - 1, compute(-1, 1));
  EXPECT_EQ(16 - 0, compute(16, 0));
  EXPECT_EQ(-70 - -1000, compute(-70, -1000));
  EXPECT_EQ(5 - 8, compute(5, 8));
}

TEST(ASMBackendTest, I64_MUL) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I64Type(), {program.I64Type(), program.I64Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.MulI64(args[0], args[1]);
  program.Return(sum);

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int64_t(int64_t, int64_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(0 * 0, compute(0, 0));
  EXPECT_EQ(-1 * 1, compute(-1, 1));
  EXPECT_EQ(16 * 0, compute(16, 0));
  EXPECT_EQ(-7 * -13, compute(-7, -13));
  EXPECT_EQ(5 * 8, compute(5, 8));
}

TEST(ASMBackendTest, I64_CONST) {
  for (auto x : {-100, 255, 17, 91}) {
    int64_t c = x;

    khir::ProgramBuilder program;
    program.CreatePublicFunction(program.I64Type(), {}, "compute");
    program.Return(program.ConstI64(c));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int64_t()>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    EXPECT_EQ(c, compute());
  }
}

TEST(ASMBackendTest, I64_CONV_F64) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(program.F64Type(),
                                           {program.I64Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.F64ConvI64(args[0]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<double(int64_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(0.0, compute(0.0));
  EXPECT_EQ(127.0, compute(127));
  EXPECT_EQ(-255.0, compute(-255));
  EXPECT_EQ(15.0, compute(15));
  EXPECT_EQ(-1.0, compute(-1));
}

TEST(ASMBackendTest, I64_CMP_EQ_Return) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.I64Type(), program.I64Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.CmpI64(khir::CompType::EQ, args[0], args[1]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(int64_t, int64_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_NE(0, compute(0, 0));
  EXPECT_EQ(0, compute(0, 1));
  EXPECT_EQ(0, compute(1, 0));
  EXPECT_NE(0, compute(1, 1));
  EXPECT_NE(0, compute(-1, -1));
  EXPECT_EQ(0, compute(-1, 0));
}

TEST(ASMBackendTest, I64_CMP_NE_Return) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.I64Type(), program.I64Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.CmpI64(khir::CompType::NE, args[0], args[1]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(int64_t, int64_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(0, compute(0, 0));
  EXPECT_NE(0, compute(0, 1));
  EXPECT_NE(0, compute(1, 0));
  EXPECT_EQ(0, compute(1, 1));
  EXPECT_EQ(0, compute(-1, -1));
  EXPECT_NE(0, compute(-1, 0));
}

TEST(ASMBackendTest, I64_CMP_LT_Return) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.I64Type(), program.I64Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.CmpI64(khir::CompType::LT, args[0], args[1]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(int64_t, int64_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(0, compute(0, 0));
  EXPECT_NE(0, compute(0, 1));
  EXPECT_EQ(0, compute(1, 0));
  EXPECT_EQ(0, compute(1, 1));
  EXPECT_EQ(0, compute(-1, -1));
  EXPECT_NE(0, compute(-1, 0));
}

TEST(ASMBackendTest, I64_CMP_GT_Return) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.I64Type(), program.I64Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.CmpI64(khir::CompType::GT, args[0], args[1]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(int64_t, int64_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(0, compute(0, 0));
  EXPECT_EQ(0, compute(0, 1));
  EXPECT_NE(0, compute(1, 0));
  EXPECT_EQ(0, compute(1, 1));
  EXPECT_EQ(0, compute(-1, -1));
  EXPECT_EQ(0, compute(-1, 0));
}

TEST(ASMBackendTest, I64_CMP_LE_Return) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.I64Type(), program.I64Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.CmpI64(khir::CompType::LE, args[0], args[1]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(int64_t, int64_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_NE(0, compute(0, 0));
  EXPECT_NE(0, compute(0, 1));
  EXPECT_EQ(0, compute(1, 0));
  EXPECT_NE(0, compute(1, 1));
  EXPECT_NE(0, compute(-1, -1));
  EXPECT_NE(0, compute(-1, 0));
}

TEST(ASMBackendTest, I64_CMP_GE_Return) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.I64Type(), program.I64Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.CmpI64(khir::CompType::GE, args[0], args[1]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(int64_t, int64_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_NE(0, compute(0, 0));
  EXPECT_EQ(0, compute(0, 1));
  EXPECT_NE(0, compute(1, 0));
  EXPECT_NE(0, compute(1, 1));
  EXPECT_NE(0, compute(-1, -1));
  EXPECT_EQ(0, compute(-1, 0));
}

TEST(ASMBackendTest, I64_LOAD) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I64Type(), {program.PointerType(program.I64Type())}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.LoadI64(args[0]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int64_t(int64_t*)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  int64_t loc;
  for (int i = -10; i <= 10; i++) {
    loc = 2 * i;
    EXPECT_EQ(loc, compute(&loc));
  }
}

TEST(ASMBackendTest, I64_STORE) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.VoidType(),
      {program.PointerType(program.I64Type()), program.I64Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.StoreI64(args[0], args[1]);
  program.Return();

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<void(int64_t*, int64_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  int64_t loc;
  for (int i = -10; i <= 10; i++) {
    compute(&loc, 2 * i);
    EXPECT_EQ(loc, 2 * i);
  }
}

TEST(ASMBackendTest, F64_ADD) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.F64Type(), {program.F64Type(), program.F64Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.AddF64(args[0], args[1]);
  program.Return(sum);

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<double(double, double)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(0.0 + 0.0, compute(0.0, 0.0));
  EXPECT_EQ(-1.0 + 1.0, compute(-1.0, 1.0));
  EXPECT_EQ(16.0 + 0.0, compute(16.0, 0.0));
  EXPECT_EQ(-70.0 + -1000.0, compute(-70.0, -1000.0));
  EXPECT_EQ(5.0 + 8.0, compute(5.0, 8.0));
}

TEST(ASMBackendTest, F64_SUB) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.F64Type(), {program.F64Type(), program.F64Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.SubF64(args[0], args[1]);
  program.Return(sum);

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<double(double, double)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(0.0 - 0.0, compute(0.0, 0.0));
  EXPECT_EQ(-1.0 - 1.0, compute(-1.0, 1.0));
  EXPECT_EQ(16.0 - 0.0, compute(16.0, 0.0));
  EXPECT_EQ(-70.0 - -1000.0, compute(-70.0, -1000.0));
  EXPECT_EQ(5.0 - 8.0, compute(5.0, 8.0));
}

TEST(ASMBackendTest, F64_MUL) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.F64Type(), {program.F64Type(), program.F64Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.MulF64(args[0], args[1]);
  program.Return(sum);

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<double(double, double)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(0.0 * 0.0, compute(0.0, 0.0));
  EXPECT_EQ(-1.0 * 1.0, compute(-1.0, 1.0));
  EXPECT_EQ(16.0 * 0.0, compute(16.0, 0.0));
  EXPECT_EQ(-7.0 * -13.0, compute(-7.0, -13.0));
  EXPECT_EQ(5.0 * 8.0, compute(5.0, 8.0));
}

TEST(ASMBackendTest, F64_CONST) {
  for (auto x : {-100.0, 255.0, 17.0, 91.0, 0.0}) {
    double c = x;

    khir::ProgramBuilder program;
    program.CreatePublicFunction(program.F64Type(), {}, "compute");
    program.Return(program.ConstF64(c));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<double()>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    EXPECT_EQ(c, compute());
  }
}

TEST(ASMBackendTest, F64_CONV_I64) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(program.I64Type(),
                                           {program.F64Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.I64ConvF64(args[0]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int64_t(double)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(0, compute(0.0));
  EXPECT_EQ(127, compute(127.2));
  EXPECT_EQ(-255, compute(-255.10));
  EXPECT_EQ(15, compute(15.1230987));
  EXPECT_EQ(3, compute(3.14159));
}

TEST(ASMBackendTest, F64_CMP_EQ_Return) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.F64Type(), program.F64Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.CmpF64(khir::CompType::EQ, args[0], args[1]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(double, double)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_NE(0, compute(0.0, 0.0));
  EXPECT_EQ(0, compute(0, 1.0));
  EXPECT_EQ(0, compute(1.0, 0));
  EXPECT_NE(0, compute(1.0, 1.0));
  EXPECT_NE(0, compute(-1.0, -1.0));
  EXPECT_EQ(0, compute(-1.0, 0.0));
}

TEST(ASMBackendTest, F64_CMP_NE_Return) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.F64Type(), program.F64Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.CmpF64(khir::CompType::NE, args[0], args[1]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(double, double)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(0, compute(0.0, 0.0));
  EXPECT_NE(0, compute(0.0, 1.0));
  EXPECT_NE(0, compute(1.0, 0.0));
  EXPECT_EQ(0, compute(1.0, 1.0));
  EXPECT_EQ(0, compute(-1.0, -1.0));
  EXPECT_NE(0, compute(-1.0, 0.0));
}

TEST(ASMBackendTest, F64_CMP_LT_Return) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.F64Type(), program.F64Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.CmpF64(khir::CompType::LT, args[0], args[1]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(double, double)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(0, compute(0.0, 0.0));
  EXPECT_NE(0, compute(0.0, 1.0));
  EXPECT_EQ(0, compute(1.0, 0.0));
  EXPECT_EQ(0, compute(1.0, 1.0));
  EXPECT_EQ(0, compute(-1.0, -1.0));
  EXPECT_NE(0, compute(-1.0, 0.0));
}

TEST(ASMBackendTest, F64_CMP_GT_Return) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.F64Type(), program.F64Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.CmpF64(khir::CompType::GT, args[0], args[1]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(double, double)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(0, compute(0.0, 0.0));
  EXPECT_EQ(0, compute(0.0, 1.0));
  EXPECT_NE(0, compute(1.0, 0.0));
  EXPECT_EQ(0, compute(1.0, 1.0));
  EXPECT_EQ(0, compute(-1.0, -1.0));
  EXPECT_EQ(0, compute(-1.0, 0.0));
}

TEST(ASMBackendTest, F64_CMP_LE_Return) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.F64Type(), program.F64Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.CmpF64(khir::CompType::LE, args[0], args[1]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(double, double)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_NE(0, compute(0.0, 0.0));
  EXPECT_NE(0, compute(0.0, 1.0));
  EXPECT_EQ(0, compute(1.0, 0.0));
  EXPECT_NE(0, compute(1.0, 1.0));
  EXPECT_NE(0, compute(-1.0, -1.0));
  EXPECT_NE(0, compute(-1.0, 0.0));
}

TEST(ASMBackendTest, F64_CMP_GE_Return) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.F64Type(), program.F64Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.CmpF64(khir::CompType::GE, args[0], args[1]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(double, double)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_NE(0, compute(0.0, 0.0));
  EXPECT_EQ(0, compute(0.0, 1.0));
  EXPECT_NE(0, compute(1.0, 0.0));
  EXPECT_NE(0, compute(1.0, 1.0));
  EXPECT_NE(0, compute(-1.0, -1.0));
  EXPECT_EQ(0, compute(-1.0, 0.0));
}

TEST(ASMBackendTest, F64_LOAD) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.F64Type(), {program.PointerType(program.F64Type())}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.LoadF64(args[0]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<double(double*)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  double loc;
  for (int i = -10; i <= 10; i++) {
    loc = 2 * i;
    EXPECT_EQ(loc, compute(&loc));
  }
}

TEST(ASMBackendTest, F64_STORE) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.VoidType(),
      {program.PointerType(program.F64Type()), program.F64Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.StoreF64(args[0], args[1]);
  program.Return();

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<void(double*, double)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  double loc;
  for (int i = -10; i <= 10; i++) {
    compute(&loc, 2 * i);
    EXPECT_EQ(loc, 2 * i);
  }
}