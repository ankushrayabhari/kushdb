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

TEST(ASMBackendTest, NULLPTR) {
  for (auto type_func :
       {&khir::ProgramBuilder::I1Type, &khir::ProgramBuilder::I8Type,
        &khir::ProgramBuilder::I16Type, &khir::ProgramBuilder::I32Type,
        &khir::ProgramBuilder::I64Type, &khir::ProgramBuilder::F64Type}) {
    khir::ProgramBuilder program;
    auto type = program.PointerType(std::invoke(type_func, program));
    program.CreatePublicFunction(type, {}, "compute");
    program.Return(program.NullPtr(type));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int8_t*()>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));
    EXPECT_EQ(nullptr, compute());
  }
}

TEST(ASMBackendTest, PTR_CAST) {
  khir::ProgramBuilder program;
  auto type = program.PointerType(program.I8Type());
  auto func = program.CreatePublicFunction(
      program.PointerType(program.F64Type()), {type}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(
      program.PointerCast(args[0], program.PointerType(program.F64Type())));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<double*(int8_t*)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));
  EXPECT_EQ(nullptr, compute(nullptr));

  int8_t test;
  EXPECT_EQ(reinterpret_cast<double*>(&test), compute(&test));
}

TEST(ASMBackendTest, PTR_CASTGlobal) {
  khir::ProgramBuilder program;
  auto global =
      program.Global(false, true, program.I64Type(), program.ConstI64(5));
  program.CreatePublicFunction(program.PointerType(program.F64Type()), {},
                               "compute");
  program.Return(
      program.PointerCast(global, program.PointerType(program.F64Type())));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<double*()>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(5, *reinterpret_cast<int64_t*>(compute()));
}

TEST(ASMBackendTest, PTR_CASTStruct) {
  struct Test {
    int64_t x;
  };

  khir::ProgramBuilder program;
  auto st = program.StructType({program.I64Type()});
  auto func =
      program.CreatePublicFunction(program.PointerType(program.F64Type()),
                                   {program.PointerType(st)}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.PointerCast(program.GetElementPtr(st, args[0], {0, 0}),
                                     program.PointerType(program.F64Type())));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<double*(Test*)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  Test test{.x = 5};
  EXPECT_EQ(5, *reinterpret_cast<int64_t*>(compute(&test)));
}

TEST(ASMBackendTest, ALLOCAI8) {
  khir::ProgramBuilder program;
  auto type = program.I8Type();
  auto func = program.CreatePublicFunction(type, {type}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto ptr = program.Alloca(type);
  program.StoreI8(ptr, args[0]);
  program.Return(program.LoadI8(ptr));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(int8_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    int8_t c = distrib(gen);
    EXPECT_EQ(c, compute(c));
  }
}

TEST(ASMBackendTest, ALLOCAI16) {
  khir::ProgramBuilder program;
  auto type = program.I16Type();
  auto func = program.CreatePublicFunction(type, {type}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto ptr = program.Alloca(type);
  program.StoreI16(ptr, args[0]);
  program.Return(program.LoadI16(ptr));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int16_t(int16_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    int16_t c = distrib(gen);
    EXPECT_EQ(c, compute(c));
  }
}

TEST(ASMBackendTest, ALLOCAI32) {
  khir::ProgramBuilder program;
  auto type = program.I32Type();
  auto func = program.CreatePublicFunction(type, {type}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto ptr = program.Alloca(type);
  program.StoreI32(ptr, args[0]);
  program.Return(program.LoadI32(ptr));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int32_t(int32_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    int32_t c = distrib(gen);
    EXPECT_EQ(c, compute(c));
  }
}

TEST(ASMBackendTest, ALLOCAI64) {
  khir::ProgramBuilder program;
  auto type = program.I64Type();
  auto func = program.CreatePublicFunction(type, {type}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto ptr = program.Alloca(type);
  program.StoreI64(ptr, args[0]);
  program.Return(program.LoadI64(ptr));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int64_t(int64_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    int64_t c = distrib(gen);
    EXPECT_EQ(c, compute(c));
  }
}

TEST(ASMBackendTest, ALLOCAF64) {
  khir::ProgramBuilder program;
  auto type = program.F64Type();
  auto func = program.CreatePublicFunction(type, {type}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto ptr = program.Alloca(type);
  program.StoreF64(ptr, args[0]);
  program.Return(program.LoadF64(ptr));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<double(double)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    double c = distrib(gen);
    EXPECT_EQ(c, compute(c));
  }
}

TEST(ASMBackendTest, ALLOCAStruct) {
  struct Test {
    int8_t x1;
    int16_t x2;
    int64_t x3;
  };

  khir::ProgramBuilder program;
  auto test_type = program.StructType(
      {program.I8Type(), program.I16Type(), program.I64Type()});

  auto handler_type = program.FunctionType(program.VoidType(),
                                           {program.PointerType(test_type)});
  auto handler_pointer_type = program.PointerType(handler_type);

  auto func = program.CreatePublicFunction(program.VoidType(),
                                           {handler_pointer_type}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto ptr = program.Alloca(test_type);
  program.StoreI8(program.GetElementPtr(test_type, ptr, {0, 0}),
                  program.ConstI8(0));
  program.StoreI16(program.GetElementPtr(test_type, ptr, {0, 1}),
                   program.ConstI16(1000));
  program.StoreI64(program.GetElementPtr(test_type, ptr, {0, 2}),
                   program.ConstI64(2));
  program.Call(args[0], {ptr});
  program.Return();

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using handler_fn = std::add_pointer<void(Test*)>::type;
  using compute_fn = std::add_pointer<void(handler_fn)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  compute([](Test* t) {
    EXPECT_EQ(0, t->x1);
    EXPECT_EQ(1000, t->x2);
    EXPECT_EQ(2, t->x3);
  });
}

TEST(ASMBackendTest, PTR_LOAD) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.PointerType(program.I64Type()),
      {program.PointerType(program.PointerType(program.I64Type()))}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.LoadPtr(args[0]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int64_t*(int64_t**)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  int64_t value = 0xDEADBEEF;
  int64_t* ptr = reinterpret_cast<int64_t*>(value);
  EXPECT_EQ(ptr, compute(&ptr));
}

TEST(ASMBackendTest, PTR_LOADGlobal) {
  khir::ProgramBuilder program;
  auto global =
      program.Global(true, false, program.PointerType(program.I64Type()),
                     program.NullPtr(program.I64Type()));
  program.CreatePublicFunction(
      program.PointerType(program.PointerType(program.I64Type())), {},
      "compute");
  program.Return(program.LoadPtr(global));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int64_t*()>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(nullptr, compute());
}

TEST(ASMBackendTest, PTR_LOADStruct) {
  struct Test {
    int64_t* x;
  };

  khir::ProgramBuilder program;
  auto st = program.StructType(
      {program.PointerType(program.PointerType(program.I64Type()))});
  auto func =
      program.CreatePublicFunction(program.PointerType(program.I64Type()),
                                   {program.PointerType(st)}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.LoadPtr(program.GetElementPtr(st, args[0], {0, 0})));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int64_t*(Test*)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  Test t{.x = reinterpret_cast<int64_t*>(0xDEADBEEF)};
  EXPECT_EQ(t.x, compute(&t));
}

TEST(ASMBackendTest, PTR_STORE) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.VoidType(),
      {program.PointerType(program.PointerType(program.I64Type())),
       program.PointerType(program.I64Type())},
      "compute");
  auto args = program.GetFunctionArguments(func);
  program.StorePtr(args[0], args[1]);
  program.Return();

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<void(int64_t**, int64_t*)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  int64_t value = 0xDEADBEEF;
  int64_t* ptr = reinterpret_cast<int64_t*>(value);
  int64_t* loc;
  compute(&loc, ptr);
  EXPECT_EQ(ptr, loc);
}

TEST(ASMBackendTest, PTR_STORENullptr) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.VoidType(),
      {program.PointerType(program.PointerType(program.I64Type()))}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.StorePtr(args[0], program.NullPtr(program.I64Type()));
  program.Return();

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<void(int64_t**)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  int64_t value = 0xDEADBEEF;
  int64_t* ptr = reinterpret_cast<int64_t*>(value);
  compute(&ptr);
  EXPECT_EQ(nullptr, ptr);
}

TEST(ASMBackendTest, PTR_STOREGlobal) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I64Type(),
      {program.PointerType(program.PointerType(program.I64Type()))}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto global =
      program.Global(true, false, program.I64Type(), program.ConstI64(-1));
  program.StorePtr(args[0], global);
  auto ptr = program.LoadPtr(args[0]);
  program.Return(program.LoadI64(ptr));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int64_t(int64_t**)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  int64_t* loc = nullptr;
  EXPECT_EQ(-1, compute(&loc));
}

TEST(ASMBackendTest, PTR_STOREGep) {
  struct Test {
    int32_t a;
    int64_t b;
  };

  khir::ProgramBuilder program;
  auto st = program.StructType({program.I32Type(), program.I64Type()});
  auto func = program.CreatePublicFunction(
      program.VoidType(),
      {program.PointerType(program.PointerType(program.I64Type())),
       program.PointerType(st)},
      "compute");
  auto args = program.GetFunctionArguments(func);
  program.StorePtr(args[0], program.GetElementPtr(st, args[1], {0, 1}));
  program.Return();

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<void(int64_t**, Test*)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  Test x{.a = 100, .b = -1000};
  int64_t* loc = nullptr;
  compute(&loc, &x);
  EXPECT_EQ(&x.b, loc);
}

TEST(ASMBackendTest, PTR_STOREGlobalDest) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.PointerType(program.I64Type()),
      {program.PointerType(program.I64Type())}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto global =
      program.Global(false, true, program.PointerType(program.I64Type()),
                     program.NullPtr(program.I64Type()));
  program.StorePtr(
      global,
      program.PointerCast(args[0], program.PointerType(program.I64Type())));
  program.Return(program.LoadPtr(global));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int64_t*(int64_t*)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  int64_t value = 0xDEADBEEF;
  int64_t* ptr = reinterpret_cast<int64_t*>(value);
  EXPECT_EQ(ptr, compute(ptr));
}

TEST(ASMBackendTest, PTR_STOREGepDest) {
  struct Test {
    int64_t* x;
  };

  khir::ProgramBuilder program;
  auto st = program.StructType({program.PointerType(program.I64Type())});
  auto func = program.CreatePublicFunction(
      program.VoidType(),
      {program.PointerType(st), program.PointerType(program.I64Type())},
      "compute");
  auto args = program.GetFunctionArguments(func);
  program.StorePtr(program.GetElementPtr(st, args[0], {0, 0}), args[1]);
  program.Return();

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<void(Test*, int64_t*)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  Test t{.x = nullptr};
  int64_t value = 0xDEADBEEF;
  int64_t* ptr = reinterpret_cast<int64_t*>(value);
  compute(&t, ptr);
  EXPECT_EQ(ptr, t.x);
}

TEST(ASMBackendTest, PHIPTR) {
  int64_t c1 = 0xDEADBEEF;
  int64_t* p1 = reinterpret_cast<int64_t*>(c1);
  int64_t c2 = 0xBEAFDEAD;
  int64_t* p2 = reinterpret_cast<int64_t*>(c2);

  khir::ProgramBuilder program;
  auto type = program.PointerType(program.I64Type());
  auto func = program.CreatePublicFunction(
      program.VoidType(), {program.I1Type(), type, type}, "compute");
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

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn =
      std::add_pointer<int64_t*(int8_t, int64_t*, int64_t*)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(p1, compute(1, p1, p2));
  EXPECT_EQ(p2, compute(0, p1, p2));
}

TEST(ASMBackendTest, I1_CONST) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(0, 1);
  for (int i = 0; i < 10; i++) {
    bool c = distrib(gen);

    khir::ProgramBuilder program;
    program.CreatePublicFunction(program.I1Type(), {}, "compute");
    program.Return(program.ConstI1(c));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<bool()>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    EXPECT_EQ(c, compute());
  }
}

TEST(ASMBackendTest, I1_LNOT) {
  khir::ProgramBuilder program;
  auto func = program.CreatePublicFunction(program.I1Type(), {program.I1Type()},
                                           "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.LNotI1(args[0]));

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<bool(bool)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(false, compute(true));
  EXPECT_EQ(true, compute(false));
}

TEST(ASMBackendTest, I1_LNOTConst) {
  for (auto c : {true, false}) {
    khir::ProgramBuilder program;
    program.CreatePublicFunction(program.I1Type(), {}, "compute");
    program.Return(program.LNotI1(program.ConstI1(c)));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<bool()>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    EXPECT_EQ(!c, compute());
  }
}

TEST(ASMBackendTest, I1_CMP_XXReturn) {
  for (auto cmp_type : {khir::CompType::EQ, khir::CompType::NE}) {
    int8_t c1 = 0;
    int8_t c2 = 1;

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(
        program.I1Type(), {program.I1Type(), program.I1Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.CmpI1(cmp_type, args[0], args[1]));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int8_t(int8_t, int8_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

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

TEST(ASMBackendTest, I1_CMP_XXBranch) {
  for (auto cmp_type : {khir::CompType::EQ, khir::CompType::NE}) {
    int8_t c1 = 0;
    int8_t c2 = 1;

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(
        program.I1Type(), {program.I1Type(), program.I1Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto bb1 = program.GenerateBlock();
    auto bb2 = program.GenerateBlock();
    program.Branch(program.CmpI1(cmp_type, args[0], args[1]), bb1, bb2);
    program.SetCurrentBlock(bb1);
    program.Return(program.ConstI1(true));
    program.SetCurrentBlock(bb2);
    program.Return(program.ConstI1(false));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int8_t(int8_t, int8_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

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

TEST(ASMBackendTest, I1_CMP_XXReturnConstArg0) {
  for (auto cmp_type : {khir::CompType::EQ, khir::CompType::NE}) {
    int8_t c1 = 0;
    int8_t c2 = 1;

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I1Type(),
                                             {program.I1Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.CmpI1(cmp_type, program.ConstI1(c1), args[0]));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int8_t(int8_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

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

TEST(ASMBackendTest, I1_CMP_XXReturnConstArg1) {
  for (auto cmp_type : {khir::CompType::EQ, khir::CompType::NE}) {
    int8_t c1 = 0;
    int8_t c2 = 1;

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I1Type(),
                                             {program.I1Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.CmpI1(cmp_type, args[0], program.ConstI1(c2)));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int8_t(int8_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    if (Compare(cmp_type, c1, c2)) {
      EXPECT_NE(0, compute(c1));
    } else {
      EXPECT_EQ(0, compute(c1));
    }

    if (Compare(cmp_type, c2, c2)) {
      EXPECT_NE(0, compute(c2));
    } else {
      EXPECT_EQ(0, compute(c2));
    }
  }
}

TEST(ASMBackendTest, I1_ZEXT_I64) {
  for (auto c : {true, false}) {
    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I64Type(),
                                             {program.I1Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.I64ZextI1(args[0]));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int64_t(bool)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int64_t zexted = c ? 1 : 0;
    EXPECT_EQ(zexted, compute(c));
  }
}

TEST(ASMBackendTest, I1_ZEXT_I64Const) {
  for (auto c : {true, false}) {
    khir::ProgramBuilder program;
    program.CreatePublicFunction(program.I64Type(), {}, "compute");
    program.Return(program.I64ZextI1(program.ConstI1(c)));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int64_t()>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int64_t zexted = c ? 1 : 0;
    EXPECT_EQ(zexted, compute());
  }
}

TEST(ASMBackendTest, I1_ZEXT_I8) {
  for (auto c : {true, false}) {
    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I8Type(),
                                             {program.I1Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.I8ZextI1(args[0]));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int8_t(bool)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    EXPECT_EQ(c, compute(c));
  }
}

TEST(ASMBackendTest, I1_ZEXT_I8Const) {
  for (auto c : {true, false}) {
    khir::ProgramBuilder program;
    program.CreatePublicFunction(program.I8Type(), {}, "compute");
    program.Return(program.I8ZextI1(program.ConstI1(c)));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int8_t()>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    EXPECT_EQ(c, compute());
  }
}

TEST(ASMBackendTest, PHII1) {
  bool c1 = false;
  bool c2 = true;

  khir::ProgramBuilder program;
  auto type = program.I1Type();
  auto func = program.CreatePublicFunction(
      program.VoidType(), {program.I1Type(), type, type}, "compute");
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

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(int8_t, int8_t, int8_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(c1, compute(1, c1, c2));
  EXPECT_EQ(c2, compute(0, c1, c2));
}

TEST(ASMBackendTest, PHII1ConstArg0) {
  bool c1 = false;
  bool c2 = true;

  khir::ProgramBuilder program;
  auto type = program.I1Type();
  auto func = program.CreatePublicFunction(program.VoidType(),
                                           {program.I1Type(), type}, "compute");
  auto args = program.GetFunctionArguments(func);

  auto bb1 = program.GenerateBlock();
  auto bb2 = program.GenerateBlock();
  auto bb3 = program.GenerateBlock();

  program.Branch(args[0], bb1, bb2);

  program.SetCurrentBlock(bb1);
  auto phi_1 = program.PhiMember(program.ConstI1(c1));
  program.Branch(bb3);

  program.SetCurrentBlock(bb2);
  auto phi_2 = program.PhiMember(args[1]);
  program.Branch(bb3);

  program.SetCurrentBlock(bb3);
  auto phi = program.Phi(type);
  program.UpdatePhiMember(phi, phi_1);
  program.UpdatePhiMember(phi, phi_2);
  program.Return(phi);

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(int8_t, int8_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(c1, compute(1, c2));
  EXPECT_EQ(c2, compute(0, c2));
}

TEST(ASMBackendTest, PHII1ConstArg1) {
  bool c1 = false;
  bool c2 = true;

  khir::ProgramBuilder program;
  auto type = program.I1Type();
  auto func = program.CreatePublicFunction(program.VoidType(),
                                           {program.I1Type(), type}, "compute");
  auto args = program.GetFunctionArguments(func);

  auto bb1 = program.GenerateBlock();
  auto bb2 = program.GenerateBlock();
  auto bb3 = program.GenerateBlock();

  program.Branch(args[0], bb1, bb2);

  program.SetCurrentBlock(bb1);
  auto phi_1 = program.PhiMember(args[1]);
  program.Branch(bb3);

  program.SetCurrentBlock(bb2);
  auto phi_2 = program.PhiMember(program.ConstI1(c2));
  program.Branch(bb3);

  program.SetCurrentBlock(bb3);
  auto phi = program.Phi(type);
  program.UpdatePhiMember(phi, phi_1);
  program.UpdatePhiMember(phi, phi_2);
  program.Return(phi);

  khir::ASMBackend backend;
  program.Translate(backend);
  backend.Compile();

  using compute_fn = std::add_pointer<int8_t(int8_t, int8_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

  EXPECT_EQ(c1, compute(1, c1));
  EXPECT_EQ(c2, compute(0, c1));
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

TEST(ASMBackendTest, I8_CMP_XXBranch) {
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
      auto bb1 = program.GenerateBlock();
      auto bb2 = program.GenerateBlock();
      program.Branch(program.CmpI8(cmp_type, args[0], args[1]), bb1, bb2);
      program.SetCurrentBlock(bb1);
      program.Return(program.ConstI1(true));
      program.SetCurrentBlock(bb2);
      program.Return(program.ConstI1(false));

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
    program.Return(program.LoadI8(global));

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
    program.StoreI8(global, args[0]);
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
    program.StoreI8(program.GetElementPtr(st, args[0], {0, 0}), args[1]);
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

TEST(ASMBackendTest, PHII8) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    int8_t c1 = distrib(gen);
    int8_t c2 = distrib(gen);

    khir::ProgramBuilder program;
    auto type = program.I8Type();
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.I1Type(), type, type}, "compute");
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

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int8_t(int8_t, int8_t, int8_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    EXPECT_EQ(c1, compute(1, c1, c2));
    EXPECT_EQ(c2, compute(0, c1, c2));
  }
}

TEST(ASMBackendTest, PHII8ConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    int8_t c1 = distrib(gen);
    int8_t c2 = distrib(gen);

    khir::ProgramBuilder program;
    auto type = program.I8Type();
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.I1Type(), type}, "compute");
    auto args = program.GetFunctionArguments(func);

    auto bb1 = program.GenerateBlock();
    auto bb2 = program.GenerateBlock();
    auto bb3 = program.GenerateBlock();

    program.Branch(args[0], bb1, bb2);

    program.SetCurrentBlock(bb1);
    auto phi_1 = program.PhiMember(program.ConstI8(c1));
    program.Branch(bb3);

    program.SetCurrentBlock(bb2);
    auto phi_2 = program.PhiMember(args[1]);
    program.Branch(bb3);

    program.SetCurrentBlock(bb3);
    auto phi = program.Phi(type);
    program.UpdatePhiMember(phi, phi_1);
    program.UpdatePhiMember(phi, phi_2);
    program.Return(phi);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int8_t(int8_t, int8_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    EXPECT_EQ(c1, compute(1, c2));
    EXPECT_EQ(c2, compute(0, c2));
  }
}

TEST(ASMBackendTest, PHII8ConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    int8_t c1 = distrib(gen);
    int8_t c2 = distrib(gen);

    khir::ProgramBuilder program;
    auto type = program.I8Type();
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.I1Type(), type}, "compute");
    auto args = program.GetFunctionArguments(func);

    auto bb1 = program.GenerateBlock();
    auto bb2 = program.GenerateBlock();
    auto bb3 = program.GenerateBlock();

    program.Branch(args[0], bb1, bb2);

    program.SetCurrentBlock(bb1);
    auto phi_1 = program.PhiMember(args[1]);
    program.Branch(bb3);

    program.SetCurrentBlock(bb2);
    auto phi_2 = program.PhiMember(program.ConstI8(c2));
    program.Branch(bb3);

    program.SetCurrentBlock(bb3);
    auto phi = program.Phi(type);
    program.UpdatePhiMember(phi, phi_1);
    program.UpdatePhiMember(phi, phi_2);
    program.Return(phi);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int8_t(int8_t, int8_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    EXPECT_EQ(c1, compute(1, c1));
    EXPECT_EQ(c2, compute(0, c1));
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

TEST(ASMBackendTest, I16_CMP_XXBranch) {
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
      auto bb1 = program.GenerateBlock();
      auto bb2 = program.GenerateBlock();
      program.Branch(program.CmpI16(cmp_type, args[0], args[1]), bb1, bb2);
      program.SetCurrentBlock(bb1);
      program.Return(program.ConstI1(true));
      program.SetCurrentBlock(bb2);
      program.Return(program.ConstI1(false));

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
    program.Return(program.LoadI16(global));

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
    program.StoreI16(global, args[0]);
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
    program.StoreI16(program.GetElementPtr(st, args[0], {0, 0}), args[1]);
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

TEST(ASMBackendTest, PHII16) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    int16_t c1 = distrib(gen);
    int16_t c2 = distrib(gen);

    khir::ProgramBuilder program;
    auto type = program.I16Type();
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.I1Type(), type, type}, "compute");
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

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn =
        std::add_pointer<int16_t(int8_t, int16_t, int16_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    EXPECT_EQ(c1, compute(1, c1, c2));
    EXPECT_EQ(c2, compute(0, c1, c2));
  }
}

TEST(ASMBackendTest, PHII16ConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    int16_t c1 = distrib(gen);
    int16_t c2 = distrib(gen);

    khir::ProgramBuilder program;
    auto type = program.I16Type();
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.I1Type(), type}, "compute");
    auto args = program.GetFunctionArguments(func);

    auto bb1 = program.GenerateBlock();
    auto bb2 = program.GenerateBlock();
    auto bb3 = program.GenerateBlock();

    program.Branch(args[0], bb1, bb2);

    program.SetCurrentBlock(bb1);
    auto phi_1 = program.PhiMember(program.ConstI16(c1));
    program.Branch(bb3);

    program.SetCurrentBlock(bb2);
    auto phi_2 = program.PhiMember(args[1]);
    program.Branch(bb3);

    program.SetCurrentBlock(bb3);
    auto phi = program.Phi(type);
    program.UpdatePhiMember(phi, phi_1);
    program.UpdatePhiMember(phi, phi_2);
    program.Return(phi);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int16_t(int8_t, int16_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    EXPECT_EQ(c1, compute(1, c2));
    EXPECT_EQ(c2, compute(0, c2));
  }
}

TEST(ASMBackendTest, PHII16ConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    int16_t c1 = distrib(gen);
    int16_t c2 = distrib(gen);

    khir::ProgramBuilder program;
    auto type = program.I16Type();
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.I1Type(), type}, "compute");
    auto args = program.GetFunctionArguments(func);

    auto bb1 = program.GenerateBlock();
    auto bb2 = program.GenerateBlock();
    auto bb3 = program.GenerateBlock();

    program.Branch(args[0], bb1, bb2);

    program.SetCurrentBlock(bb1);
    auto phi_1 = program.PhiMember(args[1]);
    program.Branch(bb3);

    program.SetCurrentBlock(bb2);
    auto phi_2 = program.PhiMember(program.ConstI16(c2));
    program.Branch(bb3);

    program.SetCurrentBlock(bb3);
    auto phi = program.Phi(type);
    program.UpdatePhiMember(phi, phi_1);
    program.UpdatePhiMember(phi, phi_2);
    program.Return(phi);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int16_t(int8_t, int16_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    EXPECT_EQ(c1, compute(1, c1));
    EXPECT_EQ(c2, compute(0, c1));
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

TEST(ASMBackendTest, I32_CMP_XXBranch) {
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
      auto bb1 = program.GenerateBlock();
      auto bb2 = program.GenerateBlock();
      program.Branch(program.CmpI32(cmp_type, args[0], args[1]), bb1, bb2);
      program.SetCurrentBlock(bb1);
      program.Return(program.ConstI1(true));
      program.SetCurrentBlock(bb2);
      program.Return(program.ConstI1(false));

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
    program.Return(program.LoadI32(global));

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
    program.StoreI32(global, args[0]);
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
    program.StoreI32(program.GetElementPtr(st, args[0], {0, 0}), args[1]);
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

TEST(ASMBackendTest, PHII32) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    int32_t c1 = distrib(gen);
    int32_t c2 = distrib(gen);

    khir::ProgramBuilder program;
    auto type = program.I32Type();
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.I1Type(), type, type}, "compute");
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

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn =
        std::add_pointer<int32_t(int8_t, int32_t, int32_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    EXPECT_EQ(c1, compute(1, c1, c2));
    EXPECT_EQ(c2, compute(0, c1, c2));
  }
}

TEST(ASMBackendTest, PHII32ConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    int32_t c1 = distrib(gen);
    int32_t c2 = distrib(gen);

    khir::ProgramBuilder program;
    auto type = program.I32Type();
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.I1Type(), type}, "compute");
    auto args = program.GetFunctionArguments(func);

    auto bb1 = program.GenerateBlock();
    auto bb2 = program.GenerateBlock();
    auto bb3 = program.GenerateBlock();

    program.Branch(args[0], bb1, bb2);

    program.SetCurrentBlock(bb1);
    auto phi_1 = program.PhiMember(program.ConstI32(c1));
    program.Branch(bb3);

    program.SetCurrentBlock(bb2);
    auto phi_2 = program.PhiMember(args[1]);
    program.Branch(bb3);

    program.SetCurrentBlock(bb3);
    auto phi = program.Phi(type);
    program.UpdatePhiMember(phi, phi_1);
    program.UpdatePhiMember(phi, phi_2);
    program.Return(phi);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int32_t(int8_t, int32_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    EXPECT_EQ(c1, compute(1, c2));
    EXPECT_EQ(c2, compute(0, c2));
  }
}

TEST(ASMBackendTest, PHII32ConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    int32_t c1 = distrib(gen);
    int32_t c2 = distrib(gen);

    khir::ProgramBuilder program;
    auto type = program.I32Type();
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.I1Type(), type}, "compute");
    auto args = program.GetFunctionArguments(func);

    auto bb1 = program.GenerateBlock();
    auto bb2 = program.GenerateBlock();
    auto bb3 = program.GenerateBlock();

    program.Branch(args[0], bb1, bb2);

    program.SetCurrentBlock(bb1);
    auto phi_1 = program.PhiMember(args[1]);
    program.Branch(bb3);

    program.SetCurrentBlock(bb2);
    auto phi_2 = program.PhiMember(program.ConstI32(c2));
    program.Branch(bb3);

    program.SetCurrentBlock(bb3);
    auto phi = program.Phi(type);
    program.UpdatePhiMember(phi, phi_1);
    program.UpdatePhiMember(phi, phi_2);
    program.Return(phi);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int32_t(int8_t, int32_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    EXPECT_EQ(c1, compute(1, c1));
    EXPECT_EQ(c2, compute(0, c1));
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

    using compute_fn = std::add_pointer<double()>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    double conv = c;
    EXPECT_EQ(conv, compute());
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

TEST(ASMBackendTest, I64_CMP_XXBranch) {
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

      auto bb1 = program.GenerateBlock();
      auto bb2 = program.GenerateBlock();
      program.Branch(program.CmpI64(cmp_type, args[0], args[1]), bb1, bb2);
      program.SetCurrentBlock(bb1);
      program.Return(program.ConstI1(true));
      program.SetCurrentBlock(bb2);
      program.Return(program.ConstI1(false));

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
    program.Return(program.LoadI64(global));

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
    program.StoreI64(global, args[0]);
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
    program.StoreI64(program.GetElementPtr(st, args[0], {0, 0}), args[1]);
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

TEST(ASMBackendTest, PHII64) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    int64_t c1 = distrib(gen);
    int64_t c2 = distrib(gen);

    khir::ProgramBuilder program;
    auto type = program.I64Type();
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.I1Type(), type, type}, "compute");
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

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn =
        std::add_pointer<int64_t(int8_t, int64_t, int64_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    EXPECT_EQ(c1, compute(1, c1, c2));
    EXPECT_EQ(c2, compute(0, c1, c2));
  }
}

TEST(ASMBackendTest, PHII64ConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    int64_t c1 = distrib(gen);
    int64_t c2 = distrib(gen);

    khir::ProgramBuilder program;
    auto type = program.I64Type();
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.I1Type(), type}, "compute");
    auto args = program.GetFunctionArguments(func);

    auto bb1 = program.GenerateBlock();
    auto bb2 = program.GenerateBlock();
    auto bb3 = program.GenerateBlock();

    program.Branch(args[0], bb1, bb2);

    program.SetCurrentBlock(bb1);
    auto phi_1 = program.PhiMember(program.ConstI64(c1));
    program.Branch(bb3);

    program.SetCurrentBlock(bb2);
    auto phi_2 = program.PhiMember(args[1]);
    program.Branch(bb3);

    program.SetCurrentBlock(bb3);
    auto phi = program.Phi(type);
    program.UpdatePhiMember(phi, phi_1);
    program.UpdatePhiMember(phi, phi_2);
    program.Return(phi);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int64_t(int8_t, int64_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    EXPECT_EQ(c1, compute(1, c2));
    EXPECT_EQ(c2, compute(0, c2));
  }
}

TEST(ASMBackendTest, PHII64ConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    int64_t c1 = distrib(gen);
    int64_t c2 = distrib(gen);

    khir::ProgramBuilder program;
    auto type = program.I64Type();
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.I1Type(), type}, "compute");
    auto args = program.GetFunctionArguments(func);

    auto bb1 = program.GenerateBlock();
    auto bb2 = program.GenerateBlock();
    auto bb3 = program.GenerateBlock();

    program.Branch(args[0], bb1, bb2);

    program.SetCurrentBlock(bb1);
    auto phi_1 = program.PhiMember(args[1]);
    program.Branch(bb3);

    program.SetCurrentBlock(bb2);
    auto phi_2 = program.PhiMember(program.ConstI64(c2));
    program.Branch(bb3);

    program.SetCurrentBlock(bb3);
    auto phi = program.Phi(type);
    program.UpdatePhiMember(phi, phi_1);
    program.UpdatePhiMember(phi, phi_2);
    program.Return(phi);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int64_t(int8_t, int64_t)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    EXPECT_EQ(c1, compute(1, c1));
    EXPECT_EQ(c2, compute(0, c1));
  }
}

TEST(ASMBackendTest, F64_ADD) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);

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

  for (int i = 0; i < 10; i++) {
    double a0 = distrib(gen);
    double a1 = distrib(gen);
    double res = a0 + a1;
    EXPECT_EQ(res, compute(a0, a1));
  }
}

TEST(ASMBackendTest, F64_ADDConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);

  for (int i = 0; i < 10; i++) {
    double a0 = distrib(gen);
    double a1 = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.F64Type(),
                                             {program.F64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.AddF64(program.ConstF64(a0), args[0]);
    program.Return(sum);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<double(double)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    double res = a0 + a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST(ASMBackendTest, F64_ADDConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);

  for (int i = 0; i < 10; i++) {
    double a0 = distrib(gen);
    double a1 = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.F64Type(),
                                             {program.F64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.AddF64(args[0], program.ConstF64(a1));
    program.Return(sum);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<double(double)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    double res = a0 + a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST(ASMBackendTest, F64_SUB) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);

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

  for (int i = 0; i < 10; i++) {
    double a0 = distrib(gen);
    double a1 = distrib(gen);
    double res = a0 - a1;
    EXPECT_EQ(res, compute(a0, a1));
  }
}

TEST(ASMBackendTest, F64_SubConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);

  for (int i = 0; i < 10; i++) {
    double a0 = distrib(gen);
    double a1 = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.F64Type(),
                                             {program.F64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.SubF64(program.ConstF64(a0), args[0]);
    program.Return(sum);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<double(double)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    double res = a0 - a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST(ASMBackendTest, F64_SubConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);

  for (int i = 0; i < 10; i++) {
    double a0 = distrib(gen);
    double a1 = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.F64Type(),
                                             {program.F64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.SubF64(args[0], program.ConstF64(a1));
    program.Return(sum);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<double(double)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    double res = a0 - a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST(ASMBackendTest, F64_MUL) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);

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

  for (int i = 0; i < 10; i++) {
    double a0 = distrib(gen);
    double a1 = distrib(gen);
    double res = a0 * a1;
    EXPECT_EQ(res, compute(a0, a1));
  }
}

TEST(ASMBackendTest, F64_MULConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);

  for (int i = 0; i < 10; i++) {
    double a0 = distrib(gen);
    double a1 = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.F64Type(),
                                             {program.F64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.MulF64(program.ConstF64(a0), args[0]);
    program.Return(sum);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<double(double)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    double res = a0 * a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST(ASMBackendTest, F64_MULConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);

  for (int i = 0; i < 10; i++) {
    double a0 = distrib(gen);
    double a1 = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.F64Type(),
                                             {program.F64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.MulF64(args[0], program.ConstF64(a1));
    program.Return(sum);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<double(double)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    double res = a0 * a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST(ASMBackendTest, F64_CONST) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    double c = distrib(gen);

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
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
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

    double c = distrib(gen);
    int64_t conv = c;
    EXPECT_EQ(conv, compute(c));
  }
}

TEST(ASMBackendTest, F64_CONV_I64Const) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    double c = distrib(gen);

    khir::ProgramBuilder program;
    program.CreatePublicFunction(program.I64Type(), {}, "compute");
    program.Return(program.I64ConvF64(program.ConstF64(c)));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<int64_t()>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    int64_t conv = c;
    EXPECT_EQ(conv, compute());
  }
}

TEST(ASMBackendTest, F64_CMP_XXReturn) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type :
         {khir::CompType::EQ, khir::CompType::NE, khir::CompType::LT,
          khir::CompType::LE, khir::CompType::GT, khir::CompType::GE}) {
      double c1 = distrib(gen);
      double c2 = distrib(gen);

      khir::ProgramBuilder program;
      auto func = program.CreatePublicFunction(
          program.I1Type(), {program.F64Type(), program.F64Type()}, "compute");
      auto args = program.GetFunctionArguments(func);
      program.Return(program.CmpF64(cmp_type, args[0], args[1]));

      khir::ASMBackend backend;
      program.Translate(backend);
      backend.Compile();

      using compute_fn = std::add_pointer<int8_t(double, double)>::type;
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

TEST(ASMBackendTest, F64_CMP_XXBranch) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type :
         {khir::CompType::EQ, khir::CompType::NE, khir::CompType::LT,
          khir::CompType::LE, khir::CompType::GT, khir::CompType::GE}) {
      double c1 = distrib(gen);
      double c2 = distrib(gen);

      khir::ProgramBuilder program;
      auto func = program.CreatePublicFunction(
          program.I1Type(), {program.F64Type(), program.F64Type()}, "compute");
      auto args = program.GetFunctionArguments(func);
      auto bb1 = program.GenerateBlock();
      auto bb2 = program.GenerateBlock();
      program.Branch(program.CmpF64(cmp_type, args[0], args[1]), bb1, bb2);
      program.SetCurrentBlock(bb1);
      program.Return(program.ConstI1(true));
      program.SetCurrentBlock(bb2);
      program.Return(program.ConstI1(false));

      khir::ASMBackend backend;
      program.Translate(backend);
      backend.Compile();

      using compute_fn = std::add_pointer<int8_t(double, double)>::type;
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

TEST(ASMBackendTest, F64_CMP_XXConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type :
         {khir::CompType::EQ, khir::CompType::NE, khir::CompType::LT,
          khir::CompType::LE, khir::CompType::GT, khir::CompType::GE}) {
      double c1 = distrib(gen);
      double c2 = distrib(gen);

      khir::ProgramBuilder program;
      auto func = program.CreatePublicFunction(program.I1Type(),
                                               {program.F64Type()}, "compute");
      auto args = program.GetFunctionArguments(func);
      program.Return(program.CmpF64(cmp_type, program.ConstF64(c1), args[0]));

      khir::ASMBackend backend;
      program.Translate(backend);
      backend.Compile();

      using compute_fn = std::add_pointer<int8_t(double)>::type;
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

TEST(ASMBackendTest, F64_CMP_XXConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type :
         {khir::CompType::EQ, khir::CompType::NE, khir::CompType::LT,
          khir::CompType::LE, khir::CompType::GT, khir::CompType::GE}) {
      double c1 = distrib(gen);
      double c2 = distrib(gen);

      khir::ProgramBuilder program;
      auto func = program.CreatePublicFunction(program.I1Type(),
                                               {program.F64Type()}, "compute");
      auto args = program.GetFunctionArguments(func);
      program.Return(program.CmpF64(cmp_type, args[0], program.ConstF64(c2)));

      khir::ASMBackend backend;
      program.Translate(backend);
      backend.Compile();

      using compute_fn = std::add_pointer<int8_t(double)>::type;
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

TEST(ASMBackendTest, F64_LOAD) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    double loc = distrib(gen);

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

    EXPECT_EQ(loc, compute(&loc));
  }
}

TEST(ASMBackendTest, F64_LOADGlobal) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    double c = distrib(gen);

    khir::ProgramBuilder program;
    auto global =
        program.Global(true, false, program.F64Type(), program.ConstF64(c));
    program.CreatePublicFunction(program.F64Type(), {}, "compute");
    program.Return(
        program.LoadF64(program.GetElementPtr(program.F64Type(), global, {0})));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<double()>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    EXPECT_EQ(c, compute());
  }
}

TEST(ASMBackendTest, F64_LOADStruct) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    double c = distrib(gen);

    struct Test {
      double x;
    };

    khir::ProgramBuilder program;
    auto st = program.StructType({program.F64Type()});
    auto func = program.CreatePublicFunction(
        program.F64Type(), {program.PointerType(st)}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.LoadF64(program.GetElementPtr(st, args[0], {0, 0})));

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<double(Test*)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    Test t{.x = c};
    EXPECT_EQ(c, compute(&t));
  }
}

TEST(ASMBackendTest, F64_STORE) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    double c = distrib(gen);

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
    compute(&loc, c);
    EXPECT_EQ(loc, c);
  }
}

TEST(ASMBackendTest, F64_STOREConst) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    double c = distrib(gen);

    khir::ProgramBuilder program;
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.PointerType(program.F64Type())},
        "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreF64(args[0], program.ConstF64(c));
    program.Return();

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<void(double*)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    double loc;
    compute(&loc);
    EXPECT_EQ(loc, c);
  }
}

TEST(ASMBackendTest, F64_STOREGlobal) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    double c = distrib(gen);

    khir::ProgramBuilder program;
    auto global =
        program.Global(true, false, program.F64Type(), program.ConstF64(c));
    auto func = program.CreatePublicFunction(
        program.PointerType(program.F64Type()), {program.F64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreF64(program.GetElementPtr(program.F64Type(), global, {0}),
                     args[0]);
    program.Return(global);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<double*(double)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    auto loc = compute(c);
    EXPECT_EQ(*loc, c);
  }
}

TEST(ASMBackendTest, F64_STOREStruct) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    double c = distrib(gen);

    struct Test {
      double x;
    };

    khir::ProgramBuilder program;
    auto st = program.StructType({program.F64Type()});
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.PointerType(st), program.F64Type()},
        "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreF64(program.GetElementPtr(st, args[0], {0}), args[1]);
    program.Return();

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<void(Test*, double)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    Test t;
    compute(&t, c);
    EXPECT_EQ(t.x, c);
  }
}

TEST(ASMBackendTest, PHIF64) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    double c1 = distrib(gen);
    double c2 = distrib(gen);

    khir::ProgramBuilder program;
    auto type = program.F64Type();
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.I1Type(), type, type}, "compute");
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

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<double(int8_t, double, double)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    EXPECT_EQ(c1, compute(1, c1, c2));
    EXPECT_EQ(c2, compute(0, c1, c2));
  }
}

TEST(ASMBackendTest, PHIF64ConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    double c1 = distrib(gen);
    double c2 = distrib(gen);

    khir::ProgramBuilder program;
    auto type = program.F64Type();
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.I1Type(), type}, "compute");
    auto args = program.GetFunctionArguments(func);

    auto bb1 = program.GenerateBlock();
    auto bb2 = program.GenerateBlock();
    auto bb3 = program.GenerateBlock();

    program.Branch(args[0], bb1, bb2);

    program.SetCurrentBlock(bb1);
    auto phi_1 = program.PhiMember(program.ConstF64(c1));
    program.Branch(bb3);

    program.SetCurrentBlock(bb2);
    auto phi_2 = program.PhiMember(args[1]);
    program.Branch(bb3);

    program.SetCurrentBlock(bb3);
    auto phi = program.Phi(type);
    program.UpdatePhiMember(phi, phi_1);
    program.UpdatePhiMember(phi, phi_2);
    program.Return(phi);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<double(int8_t, double)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    EXPECT_EQ(c1, compute(1, c2));
    EXPECT_EQ(c2, compute(0, c2));
  }
}

TEST(ASMBackendTest, PHIF64ConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    double c1 = distrib(gen);
    double c2 = distrib(gen);

    khir::ProgramBuilder program;
    auto type = program.F64Type();
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.I1Type(), type}, "compute");
    auto args = program.GetFunctionArguments(func);

    auto bb1 = program.GenerateBlock();
    auto bb2 = program.GenerateBlock();
    auto bb3 = program.GenerateBlock();

    program.Branch(args[0], bb1, bb2);

    program.SetCurrentBlock(bb1);
    auto phi_1 = program.PhiMember(args[1]);
    program.Branch(bb3);

    program.SetCurrentBlock(bb2);
    auto phi_2 = program.PhiMember(program.ConstF64(c2));
    program.Branch(bb3);

    program.SetCurrentBlock(bb3);
    auto phi = program.Phi(type);
    program.UpdatePhiMember(phi, phi_1);
    program.UpdatePhiMember(phi, phi_2);
    program.Return(phi);

    khir::ASMBackend backend;
    program.Translate(backend);
    backend.Compile();

    using compute_fn = std::add_pointer<double(int8_t, double)>::type;
    auto compute = reinterpret_cast<compute_fn>(backend.GetFunction("compute"));

    EXPECT_EQ(c1, compute(1, c1));
    EXPECT_EQ(c2, compute(0, c1));
  }
}