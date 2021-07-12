#include "khir/asm/asm_backend.h"

#include <gtest/gtest.h>
#include <random>

#include "khir/program_builder.h"
#include "khir/program_printer.h"

using namespace kush;

template <typename T>
bool Compare(khir::CompType cmp, T a0, T a1) {
  switch (cmp) {
    case khir::CompType::EQ:
      return a0 == a1;

    case khir::CompType::NE:
      return a0 != a1;

    case khir::CompType::LT:
      return a0 < a1;

    case khir::CompType::LE:
      return a0 <= a1;

    case khir::CompType::GT:
      return a0 > a1;

    case khir::CompType::GE:
      return a0 >= a1;
  }
}

TEST(ASMBackendTest, I8_ADD) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);

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

  for (int i = 0; i < 10; i++) {
    int8_t a0 = distrib(gen);
    int8_t a1 = distrib(gen);
    int8_t res = a0 + a1;
    EXPECT_EQ(res, compute(a0, a1));
    EXPECT_EQ(res, compute(a1, a0));
  }
}

TEST(ASMBackendTest, I8_ADDConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);

  for (int i = 0; i < 10; i++) {
    int8_t a0 = distrib(gen);
    int8_t a1 = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I8Type(),
                                             {program.I8Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.AddI8(program.ConstI8(a0), args[0]);
    program.Return(sum);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int8_t(int8_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int8_t res = a0 + a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST(ASMBackendTest, I8_ADDConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);

  for (int i = 0; i < 10; i++) {
    int8_t a0 = distrib(gen);
    int8_t a1 = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I8Type(),
                                             {program.I8Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.AddI8(args[0], program.ConstI8(a1));
    program.Return(sum);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int8_t(int8_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int8_t res = a0 + a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST(ASMBackendTest, I8_SUB) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);

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

  for (int i = 0; i < 10; i++) {
    int8_t a0 = distrib(gen);
    int8_t a1 = distrib(gen);
    int8_t res = a0 - a1;
    EXPECT_EQ(res, compute(a0, a1));
  }
}

TEST(ASMBackendTest, I8_SubConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);

  for (int i = 0; i < 10; i++) {
    int8_t a0 = distrib(gen);
    int8_t a1 = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I8Type(),
                                             {program.I8Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.SubI8(program.ConstI8(a0), args[0]);
    program.Return(sum);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int8_t(int8_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int8_t res = a0 - a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST(ASMBackendTest, I8_SubConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);

  for (int i = 0; i < 10; i++) {
    int8_t a0 = distrib(gen);
    int8_t a1 = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I8Type(),
                                             {program.I8Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.SubI8(args[0], program.ConstI8(a1));
    program.Return(sum);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int8_t(int8_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int8_t res = a0 - a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST(ASMBackendTest, I8_MUL) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);

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

  for (int i = 0; i < 10; i++) {
    int8_t a0 = distrib(gen);
    int8_t a1 = distrib(gen);
    int8_t res = a0 * a1;
    EXPECT_EQ(res, compute(a0, a1));
  }
}

TEST(ASMBackendTest, I8_MULConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);

  for (int i = 0; i < 10; i++) {
    int8_t a0 = distrib(gen);
    int8_t a1 = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I8Type(),
                                             {program.I8Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.MulI8(program.ConstI8(a0), args[0]);
    program.Return(sum);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int8_t(int8_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int8_t res = a0 * a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST(ASMBackendTest, I8_MULConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);

  for (int i = 0; i < 10; i++) {
    int8_t a0 = distrib(gen);
    int8_t a1 = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I8Type(),
                                             {program.I8Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.MulI8(args[0], program.ConstI8(a1));
    program.Return(sum);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int8_t(int8_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int8_t res = a0 * a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST(ASMBackendTest, I8_CONST) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    int8_t c = distrib(gen);

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
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
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

    int8_t c = distrib(gen);
    int64_t zexted = int64_t(c) & 0xFF;
    EXPECT_EQ(zexted, compute(c));
  }
}

TEST(ASMBackendTest, I8_ZEXT_I64Const) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    int8_t c = distrib(gen);

    khir::ProgramBuilder program;
    program.CreatePublicFunction(program.I64Type(), {program.I8Type()},
                                 "compute");
    program.Return(program.I64ZextI8(program.ConstI8(c)));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int64_t(int8_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int64_t zexted = int64_t(c) & 0xFF;
    EXPECT_EQ(zexted, compute(c));
  }
}

TEST(ASMBackendTest, I8_CONV_F64) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I64Type(),
                                             {program.I8Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.F64ConvI8(args[0]));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<double(int8_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int8_t c = distrib(gen);
    double conv = c;
    EXPECT_EQ(conv, compute(c));
  }
}

TEST(ASMBackendTest, I8_CONV_F64Const) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    int8_t c = distrib(gen);

    khir::ProgramBuilder program;
    program.CreatePublicFunction(program.I64Type(), {program.I8Type()},
                                 "compute");
    program.Return(program.F64ConvI8(program.ConstI8(c)));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<double(int8_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    double conv = c;
    EXPECT_EQ(conv, compute(c));
  }
}

TEST(ASMBackendTest, I8_CMP_XXReturn) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type :
         {khir::CompType::EQ, khir::CompType::NE, khir::CompType::LT,
          khir::CompType::LE, khir::CompType::GT, khir::CompType::GE}) {
      int8_t c1 = distrib(gen);
      int8_t c2 = distrib(gen);

      khir::ProgramBuilder program;
      auto func = program.CreatePublicFunction(
          program.I1Type(), {program.I8Type(), program.I8Type()}, "compute");
      auto args = program.GetFunctionArguments(func);
      program.Return(program.CmpI8(cmp_type, args[0], args[1]));

      khir::ASMBackend backend;
      program.Translate(backend);
      backend.Compile();

      using compute_fn = std::add_pointer<int8_t(int8_t, int8_t)>::type;
      auto compute =
          reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

      if (Compare(cmp_type, c1, c1)) {
        EXPECT_NE(0, compute(c1, c1));
      } else {
        EXPECT_EQ(0, compute(c1, c1));
      }

      if (Compare(cmp_type, c2, c2)) {
        EXPECT_NE(0, compute(c2, c2));
      } else {
        EXPECT_EQ(0, compute(c2, c2));
      }

      if (Compare(cmp_type, c1, c2)) {
        EXPECT_NE(0, compute(c1, c2));
      } else {
        EXPECT_EQ(0, compute(c1, c2));
      }

      if (Compare(cmp_type, c2, c1)) {
        EXPECT_NE(0, compute(c2, c1));
      } else {
        EXPECT_EQ(0, compute(c2, c1));
      }
    }
  }
}

TEST(ASMBackendTest, I8_CMP_XXConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type :
         {khir::CompType::EQ, khir::CompType::NE, khir::CompType::LT,
          khir::CompType::LE, khir::CompType::GT, khir::CompType::GE}) {
      int8_t c1 = distrib(gen);
      int8_t c2 = distrib(gen);

      khir::ProgramBuilder program;
      auto func = program.CreatePublicFunction(program.I1Type(),
                                               {program.I8Type()}, "compute");
      auto args = program.GetFunctionArguments(func);
      program.Return(program.CmpI8(cmp_type, program.ConstI8(c1), args[0]));

      khir::ASMBackend backend;
      program.Translate(backend);
      backend.Compile();

      using compute_fn = std::add_pointer<int8_t(int8_t)>::type;
      auto compute =
          reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

      if (Compare(cmp_type, c1, c1)) {
        EXPECT_NE(0, compute(c1));
      } else {
        EXPECT_EQ(0, compute(c1));
      }

      if (Compare(cmp_type, c1, c2)) {
        EXPECT_NE(0, compute(c2));
      } else {
        EXPECT_EQ(0, compute(c2));
      }
    }
  }
}

TEST(ASMBackendTest, I8_CMP_XXConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type :
         {khir::CompType::EQ, khir::CompType::NE, khir::CompType::LT,
          khir::CompType::LE, khir::CompType::GT, khir::CompType::GE}) {
      int8_t c1 = distrib(gen);
      int8_t c2 = distrib(gen);

      khir::ProgramBuilder program;
      auto func = program.CreatePublicFunction(program.I1Type(),
                                               {program.I8Type()}, "compute");
      auto args = program.GetFunctionArguments(func);
      program.Return(program.CmpI8(cmp_type, args[0], program.ConstI8(c2)));

      khir::ASMBackend backend;
      program.Translate(backend);
      backend.Compile();

      using compute_fn = std::add_pointer<int8_t(int8_t)>::type;
      auto compute =
          reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

      if (Compare(cmp_type, c2, c2)) {
        EXPECT_NE(0, compute(c2));
      } else {
        EXPECT_EQ(0, compute(c2));
      }

      if (Compare(cmp_type, c1, c2)) {
        EXPECT_NE(0, compute(c1));
      } else {
        EXPECT_EQ(0, compute(c1));
      }
    }
  }
}

TEST(ASMBackendTest, I8_LOAD) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    int8_t loc = distrib(gen);

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

    EXPECT_EQ(loc, compute(&loc));
  }
}

TEST(ASMBackendTest, I8_LOADGlobal) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    int8_t c = distrib(gen);

    khir::ProgramBuilder program;
    auto global =
        program.Global(true, false, program.I8Type(), program.ConstI8(c));
    program.CreatePublicFunction(program.I8Type(), {}, "compute");
    program.Return(
        program.LoadI8(program.GetElementPtr(program.I8Type(), global, {0})));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int8_t()>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    EXPECT_EQ(c, compute());
  }
}

TEST(ASMBackendTest, I8_LOADStruct) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    int8_t c = distrib(gen);

    struct Test {
      int8_t x;
    };

    khir::ProgramBuilder program;
    auto st = program.StructType({program.I8Type()});
    auto func = program.CreatePublicFunction(
        program.I8Type(), {program.PointerType(st)}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.LoadI8(program.GetElementPtr(st, args[0], {0, 0})));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int8_t(Test*)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    Test t{.x = c};
    EXPECT_EQ(c, compute(&t));
  }
}

TEST(ASMBackendTest, I8_STORE) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    int8_t c = distrib(gen);

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
    compute(&loc, c);
    EXPECT_EQ(loc, c);
  }
}

TEST(ASMBackendTest, I8_STOREConst) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    int8_t c = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.PointerType(program.I8Type())}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreI8(args[0], program.ConstI8(c));
    program.Return();

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<void(int8_t*)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int8_t loc;
    compute(&loc);
    EXPECT_EQ(loc, c);
  }
}

TEST(ASMBackendTest, I8_STOREGlobal) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    int8_t c = distrib(gen);

    khir::ProgramBuilder program;
    auto global =
        program.Global(true, false, program.I8Type(), program.ConstI8(c));
    auto func = program.CreatePublicFunction(
        program.PointerType(program.I8Type()), {program.I8Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreI8(program.GetElementPtr(program.I8Type(), global, {0}),
                    args[0]);
    program.Return(global);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int8_t*(int8_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    auto loc = compute(c);
    EXPECT_EQ(*loc, c);
  }
}

TEST(ASMBackendTest, I8_STOREStruct) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    int8_t c = distrib(gen);

    struct Test {
      int8_t x;
    };

    khir::ProgramBuilder program;
    auto st = program.StructType({program.I8Type()});
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.PointerType(st), program.I8Type()},
        "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreI8(program.GetElementPtr(st, args[0], {0}), args[1]);
    program.Return();

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<void(Test*, int8_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    Test t;
    compute(&t, c);
    EXPECT_EQ(t.x, c);
  }
}

TEST(ASMBackendTest, I16_ADD) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);

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

  for (int i = 0; i < 10; i++) {
    int16_t a0 = distrib(gen);
    int16_t a1 = distrib(gen);
    int16_t res = a0 + a1;
    EXPECT_EQ(res, compute(a0, a1));
  }
}

TEST(ASMBackendTest, I16_ADDConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);

  for (int i = 0; i < 10; i++) {
    int16_t a0 = distrib(gen);
    int16_t a1 = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I16Type(),
                                             {program.I16Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.AddI16(program.ConstI16(a0), args[0]);
    program.Return(sum);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int16_t(int16_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int16_t res = a0 + a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST(ASMBackendTest, I16_ADDConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);

  for (int i = 0; i < 10; i++) {
    int16_t a0 = distrib(gen);
    int16_t a1 = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I16Type(),
                                             {program.I16Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.AddI16(args[0], program.ConstI16(a1));
    program.Return(sum);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int16_t(int16_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int16_t res = a0 + a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST(ASMBackendTest, I16_SUB) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);

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

  for (int i = 0; i < 10; i++) {
    int16_t a0 = distrib(gen);
    int16_t a1 = distrib(gen);
    int16_t res = a0 - a1;
    EXPECT_EQ(res, compute(a0, a1));
  }
}

TEST(ASMBackendTest, I16_SubConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);

  for (int i = 0; i < 10; i++) {
    int16_t a0 = distrib(gen);
    int16_t a1 = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I16Type(),
                                             {program.I16Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.SubI16(program.ConstI16(a0), args[0]);
    program.Return(sum);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int16_t(int16_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int16_t res = a0 - a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST(ASMBackendTest, I16_SubConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);

  for (int i = 0; i < 10; i++) {
    int16_t a0 = distrib(gen);
    int16_t a1 = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I16Type(),
                                             {program.I16Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.SubI16(args[0], program.ConstI16(a1));
    program.Return(sum);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int16_t(int16_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int16_t res = a0 - a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST(ASMBackendTest, I16_MUL) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);

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

  for (int i = 0; i < 10; i++) {
    int16_t a0 = distrib(gen);
    int16_t a1 = distrib(gen);
    int16_t res = a0 * a1;
    EXPECT_EQ(res, compute(a0, a1));
  }
}

TEST(ASMBackendTest, I16_MULConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);

  for (int i = 0; i < 10; i++) {
    int16_t a0 = distrib(gen);
    int16_t a1 = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I16Type(),
                                             {program.I16Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.MulI16(program.ConstI16(a0), args[0]);
    program.Return(sum);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int16_t(int16_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int16_t res = a0 * a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST(ASMBackendTest, I16_MULConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);

  for (int i = 0; i < 10; i++) {
    int16_t a0 = distrib(gen);
    int16_t a1 = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I16Type(),
                                             {program.I16Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.MulI16(args[0], program.ConstI16(a1));
    program.Return(sum);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int16_t(int16_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int16_t res = a0 * a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST(ASMBackendTest, I16_CONST) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    int16_t c = distrib(gen);

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
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
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

    int16_t c = distrib(gen);
    int64_t zexted = int64_t(c) & 0xFFFF;
    EXPECT_EQ(zexted, compute(c));
  }
}

TEST(ASMBackendTest, I16_ZEXT_I64Const) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    int16_t c = distrib(gen);

    khir::ProgramBuilder program;
    program.CreatePublicFunction(program.I64Type(), {program.I16Type()},
                                 "compute");
    program.Return(program.I64ZextI16(program.ConstI16(c)));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int64_t(int16_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int64_t zexted = int64_t(c) & 0xFFFF;
    EXPECT_EQ(zexted, compute(c));
  }
}

TEST(ASMBackendTest, I16_CONV_F64) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I64Type(),
                                             {program.I16Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.F64ConvI16(args[0]));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<double(int16_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int16_t c = distrib(gen);
    double conv = c;
    EXPECT_EQ(conv, compute(c));
  }
}

TEST(ASMBackendTest, I16_CONV_F64Const) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    int16_t c = distrib(gen);

    khir::ProgramBuilder program;
    program.CreatePublicFunction(program.I64Type(), {program.I16Type()},
                                 "compute");
    program.Return(program.F64ConvI16(program.ConstI16(c)));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<double(int16_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    double conv = c;
    EXPECT_EQ(conv, compute(c));
  }
}

TEST(ASMBackendTest, I16_CMP_XXReturn) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type :
         {khir::CompType::EQ, khir::CompType::NE, khir::CompType::LT,
          khir::CompType::LE, khir::CompType::GT, khir::CompType::GE}) {
      int16_t c1 = distrib(gen);
      int16_t c2 = distrib(gen);

      khir::ProgramBuilder program;
      auto func = program.CreatePublicFunction(
          program.I1Type(), {program.I16Type(), program.I16Type()}, "compute");
      auto args = program.GetFunctionArguments(func);
      program.Return(program.CmpI16(cmp_type, args[0], args[1]));

      khir::ASMBackend backend;
      program.Translate(backend);
      backend.Compile();

      using compute_fn = std::add_pointer<int8_t(int16_t, int16_t)>::type;
      auto compute =
          reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

      if (Compare(cmp_type, c1, c1)) {
        EXPECT_NE(0, compute(c1, c1));
      } else {
        EXPECT_EQ(0, compute(c1, c1));
      }

      if (Compare(cmp_type, c2, c2)) {
        EXPECT_NE(0, compute(c2, c2));
      } else {
        EXPECT_EQ(0, compute(c2, c2));
      }

      if (Compare(cmp_type, c1, c2)) {
        EXPECT_NE(0, compute(c1, c2));
      } else {
        EXPECT_EQ(0, compute(c1, c2));
      }

      if (Compare(cmp_type, c2, c1)) {
        EXPECT_NE(0, compute(c2, c1));
      } else {
        EXPECT_EQ(0, compute(c2, c1));
      }
    }
  }
}

TEST(ASMBackendTest, I16_CMP_XXConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type :
         {khir::CompType::EQ, khir::CompType::NE, khir::CompType::LT,
          khir::CompType::LE, khir::CompType::GT, khir::CompType::GE}) {
      int16_t c1 = distrib(gen);
      int16_t c2 = distrib(gen);

      khir::ProgramBuilder program;
      auto func = program.CreatePublicFunction(program.I1Type(),
                                               {program.I16Type()}, "compute");
      auto args = program.GetFunctionArguments(func);
      program.Return(program.CmpI16(cmp_type, program.ConstI16(c1), args[0]));

      khir::ASMBackend backend;
      program.Translate(backend);
      backend.Compile();

      using compute_fn = std::add_pointer<int8_t(int16_t)>::type;
      auto compute =
          reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

      if (Compare(cmp_type, c1, c1)) {
        EXPECT_NE(0, compute(c1));
      } else {
        EXPECT_EQ(0, compute(c1));
      }

      if (Compare(cmp_type, c1, c2)) {
        EXPECT_NE(0, compute(c2));
      } else {
        EXPECT_EQ(0, compute(c2));
      }
    }
  }
}

TEST(ASMBackendTest, I16_CMP_XXConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type :
         {khir::CompType::EQ, khir::CompType::NE, khir::CompType::LT,
          khir::CompType::LE, khir::CompType::GT, khir::CompType::GE}) {
      int16_t c1 = distrib(gen);
      int16_t c2 = distrib(gen);

      khir::ProgramBuilder program;
      auto func = program.CreatePublicFunction(program.I1Type(),
                                               {program.I16Type()}, "compute");
      auto args = program.GetFunctionArguments(func);
      program.Return(program.CmpI16(cmp_type, args[0], program.ConstI16(c2)));

      khir::ASMBackend backend;
      program.Translate(backend);
      backend.Compile();

      using compute_fn = std::add_pointer<int8_t(int16_t)>::type;
      auto compute =
          reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

      if (Compare(cmp_type, c2, c2)) {
        EXPECT_NE(0, compute(c2));
      } else {
        EXPECT_EQ(0, compute(c2));
      }

      if (Compare(cmp_type, c1, c2)) {
        EXPECT_NE(0, compute(c1));
      } else {
        EXPECT_EQ(0, compute(c1));
      }
    }
  }
}

TEST(ASMBackendTest, I16_LOAD) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    int16_t loc = distrib(gen);

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

    EXPECT_EQ(loc, compute(&loc));
  }
}

TEST(ASMBackendTest, I16_LOADGlobal) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    int16_t c = distrib(gen);

    khir::ProgramBuilder program;
    auto global =
        program.Global(true, false, program.I16Type(), program.ConstI16(c));
    program.CreatePublicFunction(program.I16Type(), {}, "compute");
    program.Return(
        program.LoadI16(program.GetElementPtr(program.I16Type(), global, {0})));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int16_t()>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    EXPECT_EQ(c, compute());
  }
}

TEST(ASMBackendTest, I16_LOADStruct) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    int16_t c = distrib(gen);

    struct Test {
      int16_t x;
    };

    khir::ProgramBuilder program;
    auto st = program.StructType({program.I16Type()});
    auto func = program.CreatePublicFunction(
        program.I16Type(), {program.PointerType(st)}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.LoadI16(program.GetElementPtr(st, args[0], {0, 0})));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int16_t(Test*)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    Test t{.x = c};
    EXPECT_EQ(c, compute(&t));
  }
}

TEST(ASMBackendTest, I16_STORE) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    int16_t c = distrib(gen);

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
    compute(&loc, c);
    EXPECT_EQ(loc, c);
  }
}

TEST(ASMBackendTest, I16_STOREConst) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    int16_t c = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.PointerType(program.I16Type())},
        "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreI16(args[0], program.ConstI16(c));
    program.Return();

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<void(int16_t*)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int16_t loc;
    compute(&loc);
    EXPECT_EQ(loc, c);
  }
}

TEST(ASMBackendTest, I16_STOREGlobal) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    int16_t c = distrib(gen);

    khir::ProgramBuilder program;
    auto global =
        program.Global(true, false, program.I16Type(), program.ConstI16(c));
    auto func = program.CreatePublicFunction(
        program.PointerType(program.I16Type()), {program.I16Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreI16(program.GetElementPtr(program.I16Type(), global, {0}),
                     args[0]);
    program.Return(global);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int16_t*(int16_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    auto loc = compute(c);
    EXPECT_EQ(*loc, c);
  }
}

TEST(ASMBackendTest, I16_STOREStruct) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    int16_t c = distrib(gen);

    struct Test {
      int16_t x;
    };

    khir::ProgramBuilder program;
    auto st = program.StructType({program.I16Type()});
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.PointerType(st), program.I16Type()},
        "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreI16(program.GetElementPtr(st, args[0], {0}), args[1]);
    program.Return();

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<void(Test*, int16_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    Test t;
    compute(&t, c);
    EXPECT_EQ(t.x, c);
  }
}

TEST(ASMBackendTest, I32_ADD) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);

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

  for (int i = 0; i < 10; i++) {
    int32_t a0 = distrib(gen);
    int32_t a1 = distrib(gen);
    int32_t res = a0 + a1;
    EXPECT_EQ(res, compute(a0, a1));
  }
}

TEST(ASMBackendTest, I32_ADDConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);

  for (int i = 0; i < 10; i++) {
    int32_t a0 = distrib(gen);
    int32_t a1 = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I32Type(),
                                             {program.I32Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.AddI32(program.ConstI32(a0), args[0]);
    program.Return(sum);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int32_t(int32_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int32_t res = a0 + a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST(ASMBackendTest, I32_ADDConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);

  for (int i = 0; i < 10; i++) {
    int32_t a0 = distrib(gen);
    int32_t a1 = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I32Type(),
                                             {program.I32Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.AddI32(args[0], program.ConstI32(a1));
    program.Return(sum);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int32_t(int32_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int32_t res = a0 + a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST(ASMBackendTest, I32_SUB) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);

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

  for (int i = 0; i < 10; i++) {
    int32_t a0 = distrib(gen);
    int32_t a1 = distrib(gen);
    int32_t res = a0 - a1;
    EXPECT_EQ(res, compute(a0, a1));
  }
}

TEST(ASMBackendTest, I32_SubConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);

  for (int i = 0; i < 10; i++) {
    int32_t a0 = distrib(gen);
    int32_t a1 = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I32Type(),
                                             {program.I32Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.SubI32(program.ConstI32(a0), args[0]);
    program.Return(sum);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int32_t(int32_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int32_t res = a0 - a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST(ASMBackendTest, I32_SubConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);

  for (int i = 0; i < 10; i++) {
    int32_t a0 = distrib(gen);
    int32_t a1 = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I32Type(),
                                             {program.I32Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.SubI32(args[0], program.ConstI32(a1));
    program.Return(sum);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int32_t(int32_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int32_t res = a0 - a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST(ASMBackendTest, I32_MUL) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);

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

  for (int i = 0; i < 10; i++) {
    int32_t a0 = distrib(gen);
    int32_t a1 = distrib(gen);
    int32_t res = a0 * a1;
    EXPECT_EQ(res, compute(a0, a1));
  }
}

TEST(ASMBackendTest, I32_MULConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);

  for (int i = 0; i < 10; i++) {
    int32_t a0 = distrib(gen);
    int32_t a1 = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I32Type(),
                                             {program.I32Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.MulI32(program.ConstI32(a0), args[0]);
    program.Return(sum);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int32_t(int32_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int32_t res = a0 * a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST(ASMBackendTest, I32_MULConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);

  for (int i = 0; i < 10; i++) {
    int32_t a0 = distrib(gen);
    int32_t a1 = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I32Type(),
                                             {program.I32Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.MulI32(args[0], program.ConstI32(a1));
    program.Return(sum);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int32_t(int32_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int32_t res = a0 * a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST(ASMBackendTest, I32_CONST) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    int32_t c = distrib(gen);

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
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
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

    int32_t c = distrib(gen);
    int64_t zexted = int64_t(c) & 0xFFFFFFFF;
    EXPECT_EQ(zexted, compute(c));
  }
}

TEST(ASMBackendTest, I32_ZEXT_I64Const) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    int32_t c = distrib(gen);

    khir::ProgramBuilder program;
    program.CreatePublicFunction(program.I64Type(), {program.I32Type()},
                                 "compute");
    program.Return(program.I64ZextI32(program.ConstI32(c)));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int64_t(int32_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int64_t zexted = int64_t(c) & 0xFFFFFFFF;
    EXPECT_EQ(zexted, compute(c));
  }
}

TEST(ASMBackendTest, I32_CONV_F64) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I64Type(),
                                             {program.I32Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.F64ConvI32(args[0]));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<double(int32_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int32_t c = distrib(gen);
    double conv = c;
    EXPECT_EQ(conv, compute(c));
  }
}

TEST(ASMBackendTest, I32_CONV_F64Const) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    int32_t c = distrib(gen);

    khir::ProgramBuilder program;
    program.CreatePublicFunction(program.I64Type(), {program.I32Type()},
                                 "compute");
    program.Return(program.F64ConvI32(program.ConstI32(c)));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<double(int32_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    double conv = c;
    EXPECT_EQ(conv, compute(c));
  }
}

TEST(ASMBackendTest, I32_CMP_XXReturn) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type :
         {khir::CompType::EQ, khir::CompType::NE, khir::CompType::LT,
          khir::CompType::LE, khir::CompType::GT, khir::CompType::GE}) {
      int32_t c1 = distrib(gen);
      int32_t c2 = distrib(gen);

      khir::ProgramBuilder program;
      auto func = program.CreatePublicFunction(
          program.I1Type(), {program.I32Type(), program.I32Type()}, "compute");
      auto args = program.GetFunctionArguments(func);
      program.Return(program.CmpI32(cmp_type, args[0], args[1]));

      khir::ASMBackend backend;
      program.Translate(backend);
      backend.Compile();

      using compute_fn = std::add_pointer<int8_t(int32_t, int32_t)>::type;
      auto compute =
          reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

      if (Compare(cmp_type, c1, c1)) {
        EXPECT_NE(0, compute(c1, c1));
      } else {
        EXPECT_EQ(0, compute(c1, c1));
      }

      if (Compare(cmp_type, c2, c2)) {
        EXPECT_NE(0, compute(c2, c2));
      } else {
        EXPECT_EQ(0, compute(c2, c2));
      }

      if (Compare(cmp_type, c1, c2)) {
        EXPECT_NE(0, compute(c1, c2));
      } else {
        EXPECT_EQ(0, compute(c1, c2));
      }

      if (Compare(cmp_type, c2, c1)) {
        EXPECT_NE(0, compute(c2, c1));
      } else {
        EXPECT_EQ(0, compute(c2, c1));
      }
    }
  }
}

TEST(ASMBackendTest, I32_CMP_XXConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type :
         {khir::CompType::EQ, khir::CompType::NE, khir::CompType::LT,
          khir::CompType::LE, khir::CompType::GT, khir::CompType::GE}) {
      int32_t c1 = distrib(gen);
      int32_t c2 = distrib(gen);

      khir::ProgramBuilder program;
      auto func = program.CreatePublicFunction(program.I1Type(),
                                               {program.I32Type()}, "compute");
      auto args = program.GetFunctionArguments(func);
      program.Return(program.CmpI32(cmp_type, program.ConstI32(c1), args[0]));

      khir::ASMBackend backend;
      program.Translate(backend);
      backend.Compile();

      using compute_fn = std::add_pointer<int8_t(int32_t)>::type;
      auto compute =
          reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

      if (Compare(cmp_type, c1, c1)) {
        EXPECT_NE(0, compute(c1));
      } else {
        EXPECT_EQ(0, compute(c1));
      }

      if (Compare(cmp_type, c1, c2)) {
        EXPECT_NE(0, compute(c2));
      } else {
        EXPECT_EQ(0, compute(c2));
      }
    }
  }
}

TEST(ASMBackendTest, I32_CMP_XXConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type :
         {khir::CompType::EQ, khir::CompType::NE, khir::CompType::LT,
          khir::CompType::LE, khir::CompType::GT, khir::CompType::GE}) {
      int32_t c1 = distrib(gen);
      int32_t c2 = distrib(gen);

      khir::ProgramBuilder program;
      auto func = program.CreatePublicFunction(program.I1Type(),
                                               {program.I32Type()}, "compute");
      auto args = program.GetFunctionArguments(func);
      program.Return(program.CmpI32(cmp_type, args[0], program.ConstI32(c2)));

      khir::ASMBackend backend;
      program.Translate(backend);
      backend.Compile();

      using compute_fn = std::add_pointer<int8_t(int32_t)>::type;
      auto compute =
          reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

      if (Compare(cmp_type, c2, c2)) {
        EXPECT_NE(0, compute(c2));
      } else {
        EXPECT_EQ(0, compute(c2));
      }

      if (Compare(cmp_type, c1, c2)) {
        EXPECT_NE(0, compute(c1));
      } else {
        EXPECT_EQ(0, compute(c1));
      }
    }
  }
}

TEST(ASMBackendTest, I32_LOAD) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    int32_t loc = distrib(gen);

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

    EXPECT_EQ(loc, compute(&loc));
  }
}

TEST(ASMBackendTest, I32_LOADGlobal) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    int32_t c = distrib(gen);

    khir::ProgramBuilder program;
    auto global =
        program.Global(true, false, program.I32Type(), program.ConstI32(c));
    program.CreatePublicFunction(program.I32Type(), {}, "compute");
    program.Return(
        program.LoadI32(program.GetElementPtr(program.I32Type(), global, {0})));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int32_t()>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    EXPECT_EQ(c, compute());
  }
}

TEST(ASMBackendTest, I32_LOADStruct) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    int32_t c = distrib(gen);

    struct Test {
      int32_t x;
    };

    khir::ProgramBuilder program;
    auto st = program.StructType({program.I32Type()});
    auto func = program.CreatePublicFunction(
        program.I32Type(), {program.PointerType(st)}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.LoadI32(program.GetElementPtr(st, args[0], {0, 0})));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int32_t(Test*)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    Test t{.x = c};
    EXPECT_EQ(c, compute(&t));
  }
}

TEST(ASMBackendTest, I32_STORE) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    int32_t c = distrib(gen);

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
    compute(&loc, c);
    EXPECT_EQ(loc, c);
  }
}

TEST(ASMBackendTest, I32_STOREConst) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    int32_t c = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.PointerType(program.I32Type())},
        "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreI32(args[0], program.ConstI32(c));
    program.Return();

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<void(int32_t*)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int32_t loc;
    compute(&loc);
    EXPECT_EQ(loc, c);
  }
}

TEST(ASMBackendTest, I32_STOREGlobal) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    int32_t c = distrib(gen);

    khir::ProgramBuilder program;
    auto global =
        program.Global(true, false, program.I32Type(), program.ConstI32(c));
    auto func = program.CreatePublicFunction(
        program.PointerType(program.I32Type()), {program.I32Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreI32(program.GetElementPtr(program.I32Type(), global, {0}),
                     args[0]);
    program.Return(global);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int32_t*(int32_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    auto loc = compute(c);
    EXPECT_EQ(*loc, c);
  }
}

TEST(ASMBackendTest, I32_STOREStruct) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    int32_t c = distrib(gen);

    struct Test {
      int32_t x;
    };

    khir::ProgramBuilder program;
    auto st = program.StructType({program.I32Type()});
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.PointerType(st), program.I32Type()},
        "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreI32(program.GetElementPtr(st, args[0], {0}), args[1]);
    program.Return();

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<void(Test*, int32_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    Test t;
    compute(&t, c);
    EXPECT_EQ(t.x, c);
  }
}

TEST(ASMBackendTest, I64_ADD) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);

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

  for (int i = 0; i < 10; i++) {
    int64_t a0 = distrib(gen);
    int64_t a1 = distrib(gen);
    int64_t res = a0 + a1;
    EXPECT_EQ(res, compute(a0, a1));
  }
}

TEST(ASMBackendTest, I64_ADDConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);

  for (int i = 0; i < 10; i++) {
    int64_t a0 = distrib(gen);
    int64_t a1 = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I64Type(),
                                             {program.I64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.AddI64(program.ConstI64(a0), args[0]);
    program.Return(sum);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int64_t(int64_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int64_t res = a0 + a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST(ASMBackendTest, I64_ADDConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);

  for (int i = 0; i < 10; i++) {
    int64_t a0 = distrib(gen);
    int64_t a1 = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I64Type(),
                                             {program.I64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.AddI64(args[0], program.ConstI64(a1));
    program.Return(sum);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int64_t(int64_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int64_t res = a0 + a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST(ASMBackendTest, I64_SUB) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);

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

  for (int i = 0; i < 10; i++) {
    int64_t a0 = distrib(gen);
    int64_t a1 = distrib(gen);
    int64_t res = a0 - a1;
    EXPECT_EQ(res, compute(a0, a1));
  }
}

TEST(ASMBackendTest, I64_SubConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);

  for (int i = 0; i < 10; i++) {
    int64_t a0 = distrib(gen);
    int64_t a1 = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I64Type(),
                                             {program.I64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.SubI64(program.ConstI64(a0), args[0]);
    program.Return(sum);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int64_t(int64_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int64_t res = a0 - a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST(ASMBackendTest, I64_SubConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);

  for (int i = 0; i < 10; i++) {
    int64_t a0 = distrib(gen);
    int64_t a1 = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I64Type(),
                                             {program.I64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.SubI64(args[0], program.ConstI64(a1));
    program.Return(sum);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int64_t(int64_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int64_t res = a0 - a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST(ASMBackendTest, I64_MUL) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);

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

  for (int i = 0; i < 10; i++) {
    int64_t a0 = distrib(gen);
    int64_t a1 = distrib(gen);
    int64_t res = a0 * a1;
    EXPECT_EQ(res, compute(a0, a1));
  }
}

TEST(ASMBackendTest, I64_MULConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);

  for (int i = 0; i < 10; i++) {
    int64_t a0 = distrib(gen);
    int64_t a1 = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I64Type(),
                                             {program.I64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.MulI64(program.ConstI64(a0), args[0]);
    program.Return(sum);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int64_t(int64_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int64_t res = a0 * a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST(ASMBackendTest, I64_MULConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);

  for (int i = 0; i < 10; i++) {
    int64_t a0 = distrib(gen);
    int64_t a1 = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I64Type(),
                                             {program.I64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.MulI64(args[0], program.ConstI64(a1));
    program.Return(sum);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int64_t(int64_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int64_t res = a0 * a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST(ASMBackendTest, I64_CONST) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    int64_t c = distrib(gen);

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
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I64Type(),
                                             {program.I64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.F64ConvI64(args[0]));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<double(int64_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int64_t c = distrib(gen);
    double conv = c;
    EXPECT_EQ(conv, compute(c));
  }
}

TEST(ASMBackendTest, I64_CONV_F64Const) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    int64_t c = distrib(gen);

    khir::ProgramBuilder program;
    program.CreatePublicFunction(program.I64Type(), {program.I64Type()},
                                 "compute");
    program.Return(program.F64ConvI64(program.ConstI64(c)));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<double(int64_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    double conv = c;
    EXPECT_EQ(conv, compute(c));
  }
}

TEST(ASMBackendTest, I64_CMP_XXReturn) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type :
         {khir::CompType::EQ, khir::CompType::NE, khir::CompType::LT,
          khir::CompType::LE, khir::CompType::GT, khir::CompType::GE}) {
      int64_t c1 = distrib(gen);
      int64_t c2 = distrib(gen);

      khir::ProgramBuilder program;
      auto func = program.CreatePublicFunction(
          program.I1Type(), {program.I64Type(), program.I64Type()}, "compute");
      auto args = program.GetFunctionArguments(func);
      program.Return(program.CmpI64(cmp_type, args[0], args[1]));

      khir::ASMBackend backend;
      program.Translate(backend);
      backend.Compile();

      using compute_fn = std::add_pointer<int8_t(int64_t, int64_t)>::type;
      auto compute =
          reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

      if (Compare(cmp_type, c1, c1)) {
        EXPECT_NE(0, compute(c1, c1));
      } else {
        EXPECT_EQ(0, compute(c1, c1));
      }

      if (Compare(cmp_type, c2, c2)) {
        EXPECT_NE(0, compute(c2, c2));
      } else {
        EXPECT_EQ(0, compute(c2, c2));
      }

      if (Compare(cmp_type, c1, c2)) {
        EXPECT_NE(0, compute(c1, c2));
      } else {
        EXPECT_EQ(0, compute(c1, c2));
      }

      if (Compare(cmp_type, c2, c1)) {
        EXPECT_NE(0, compute(c2, c1));
      } else {
        EXPECT_EQ(0, compute(c2, c1));
      }
    }
  }
}

TEST(ASMBackendTest, I64_CMP_XXConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type :
         {khir::CompType::EQ, khir::CompType::NE, khir::CompType::LT,
          khir::CompType::LE, khir::CompType::GT, khir::CompType::GE}) {
      int64_t c1 = distrib(gen);
      int64_t c2 = distrib(gen);

      khir::ProgramBuilder program;
      auto func = program.CreatePublicFunction(program.I1Type(),
                                               {program.I64Type()}, "compute");
      auto args = program.GetFunctionArguments(func);
      program.Return(program.CmpI64(cmp_type, program.ConstI64(c1), args[0]));

      khir::ASMBackend backend;
      program.Translate(backend);
      backend.Compile();

      using compute_fn = std::add_pointer<int8_t(int64_t)>::type;
      auto compute =
          reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

      if (Compare(cmp_type, c1, c1)) {
        EXPECT_NE(0, compute(c1));
      } else {
        EXPECT_EQ(0, compute(c1));
      }

      if (Compare(cmp_type, c1, c2)) {
        EXPECT_NE(0, compute(c2));
      } else {
        EXPECT_EQ(0, compute(c2));
      }
    }
  }
}

TEST(ASMBackendTest, I64_CMP_XXConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type :
         {khir::CompType::EQ, khir::CompType::NE, khir::CompType::LT,
          khir::CompType::LE, khir::CompType::GT, khir::CompType::GE}) {
      int64_t c1 = distrib(gen);
      int64_t c2 = distrib(gen);

      khir::ProgramBuilder program;
      auto func = program.CreatePublicFunction(program.I1Type(),
                                               {program.I64Type()}, "compute");
      auto args = program.GetFunctionArguments(func);
      program.Return(program.CmpI64(cmp_type, args[0], program.ConstI64(c2)));

      khir::ASMBackend backend;
      program.Translate(backend);
      backend.Compile();

      using compute_fn = std::add_pointer<int8_t(int64_t)>::type;
      auto compute =
          reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

      if (Compare(cmp_type, c2, c2)) {
        EXPECT_NE(0, compute(c2));
      } else {
        EXPECT_EQ(0, compute(c2));
      }

      if (Compare(cmp_type, c1, c2)) {
        EXPECT_NE(0, compute(c1));
      } else {
        EXPECT_EQ(0, compute(c1));
      }
    }
  }
}

TEST(ASMBackendTest, I64_LOAD) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    int64_t loc = distrib(gen);

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

    EXPECT_EQ(loc, compute(&loc));
  }
}

TEST(ASMBackendTest, I64_LOADGlobal) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    int64_t c = distrib(gen);

    khir::ProgramBuilder program;
    auto global =
        program.Global(true, false, program.I64Type(), program.ConstI64(c));
    program.CreatePublicFunction(program.I64Type(), {}, "compute");
    program.Return(
        program.LoadI64(program.GetElementPtr(program.I64Type(), global, {0})));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int64_t()>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    EXPECT_EQ(c, compute());
  }
}

TEST(ASMBackendTest, I64_LOADStruct) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    int64_t c = distrib(gen);

    struct Test {
      int64_t x;
    };

    khir::ProgramBuilder program;
    auto st = program.StructType({program.I64Type()});
    auto func = program.CreatePublicFunction(
        program.I64Type(), {program.PointerType(st)}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.LoadI64(program.GetElementPtr(st, args[0], {0, 0})));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int64_t(Test*)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    Test t{.x = c};
    EXPECT_EQ(c, compute(&t));
  }
}

TEST(ASMBackendTest, I64_STORE) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    int64_t c = distrib(gen);

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
    compute(&loc, c);
    EXPECT_EQ(loc, c);
  }
}

TEST(ASMBackendTest, I64_STOREConst) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    int64_t c = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.PointerType(program.I64Type())},
        "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreI64(args[0], program.ConstI64(c));
    program.Return();

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<void(int64_t*)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int64_t loc;
    compute(&loc);
    EXPECT_EQ(loc, c);
  }
}

TEST(ASMBackendTest, I64_STOREGlobal) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    int64_t c = distrib(gen);

    khir::ProgramBuilder program;
    auto global =
        program.Global(true, false, program.I64Type(), program.ConstI64(c));
    auto func = program.CreatePublicFunction(
        program.PointerType(program.I64Type()), {program.I64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreI64(program.GetElementPtr(program.I64Type(), global, {0}),
                     args[0]);
    program.Return(global);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int64_t*(int64_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    auto loc = compute(c);
    EXPECT_EQ(*loc, c);
  }
}

TEST(ASMBackendTest, I64_STOREStruct) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    int64_t c = distrib(gen);

    struct Test {
      int64_t x;
    };

    khir::ProgramBuilder program;
    auto st = program.StructType({program.I64Type()});
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.PointerType(st), program.I64Type()},
        "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreI64(program.GetElementPtr(st, args[0], {0}), args[1]);
    program.Return();

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<void(Test*, int64_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    Test t;
    compute(&t, c);
    EXPECT_EQ(t.x, c);
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