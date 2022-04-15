#include "khir/backend.h"

#include <random>

#include "gtest/gtest.h"

#include "khir/asm/asm_backend.h"
#include "khir/llvm/llvm_backend.h"
#include "khir/program_builder.h"
#include "khir/program_printer.h"

using namespace kush;
using namespace kush::khir;

std::unique_ptr<Program> Compile(
    const std::pair<BackendType, khir::RegAllocImpl>& params,
    khir::ProgramBuilder& program_builder) {
  switch (params.first) {
    case BackendType::ASM: {
      auto backend = std::make_unique<khir::ASMBackend>(params.second);
      program_builder.Translate(*backend);
      backend->Compile();
      return std::move(backend);
    }

    case BackendType::LLVM: {
      auto backend = std::make_unique<khir::LLVMBackend>();
      program_builder.Translate(*backend);
      backend->Compile();
      return std::move(backend);
    }
  }
}

template <typename T>
bool Compare(CompType cmp, T a0, T a1) {
  switch (cmp) {
    case CompType::EQ:
      return a0 == a1;

    case CompType::NE:
      return a0 != a1;

    case CompType::LT:
      return a0 < a1;

    case CompType::LE:
      return a0 <= a1;

    case CompType::GT:
      return a0 > a1;

    case CompType::GE:
      return a0 >= a1;
  }
}

class BackendTest : public testing::TestWithParam<
                        std::pair<BackendType, khir::RegAllocImpl>> {};

TEST_P(BackendTest, NULLPTR) {
  for (auto type_func : {&ProgramBuilder::I1Type, &ProgramBuilder::I8Type,
                         &ProgramBuilder::I16Type, &ProgramBuilder::I32Type,
                         &ProgramBuilder::I64Type, &ProgramBuilder::F64Type}) {
    ProgramBuilder program;
    auto type = program.PointerType(std::invoke(type_func, program));
    program.CreatePublicFunction(type, {}, "compute");
    program.Return(program.NullPtr(type));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int8_t*()>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));
    EXPECT_EQ(nullptr, compute());
  }
}

TEST_P(BackendTest, PTR_CAST) {
  ProgramBuilder program;
  auto type = program.PointerType(program.I8Type());
  auto func = program.CreatePublicFunction(
      program.PointerType(program.F64Type()), {type}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(
      program.PointerCast(args[0], program.PointerType(program.F64Type())));

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<double*(int8_t*)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));
  EXPECT_EQ(nullptr, compute(nullptr));

  int8_t test;
  EXPECT_EQ(reinterpret_cast<double*>(&test), compute(&test));
}

TEST_P(BackendTest, PTR_CASTGlobal) {
  ProgramBuilder program;
  auto global =
      program.Global(false, true, program.I64Type(), program.ConstI64(5));
  program.CreatePublicFunction(program.PointerType(program.F64Type()), {},
                               "compute");
  program.Return(
      program.PointerCast(global, program.PointerType(program.F64Type())));

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<double*()>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  EXPECT_EQ(5, *reinterpret_cast<int64_t*>(compute()));
}

TEST_P(BackendTest, PTR_CASTStruct) {
  struct Test {
    int64_t x;
  };

  ProgramBuilder program;
  auto st = program.StructType({program.I64Type()});
  auto func =
      program.CreatePublicFunction(program.PointerType(program.F64Type()),
                                   {program.PointerType(st)}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.PointerCast(program.ConstGEP(st, args[0], {0, 0}),
                                     program.PointerType(program.F64Type())));

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<double*(Test*)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  Test test{.x = 5};
  EXPECT_EQ(5, *reinterpret_cast<int64_t*>(compute(&test)));
}

TEST_P(BackendTest, ALLOCAI8) {
  ProgramBuilder program;
  auto type = program.I8Type();
  auto func = program.CreatePublicFunction(type, {type}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto ptr = program.Alloca(type);
  program.StoreI8(ptr, args[0]);
  program.Return(program.LoadI8(ptr));

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<int8_t(int8_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    int8_t c = distrib(gen);
    EXPECT_EQ(c, compute(c));
  }
}

TEST_P(BackendTest, ALLOCAI16) {
  ProgramBuilder program;
  auto type = program.I16Type();
  auto func = program.CreatePublicFunction(type, {type}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto ptr = program.Alloca(type);
  program.StoreI16(ptr, args[0]);
  program.Return(program.LoadI16(ptr));

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<int16_t(int16_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    int16_t c = distrib(gen);
    EXPECT_EQ(c, compute(c));
  }
}

TEST_P(BackendTest, ALLOCAI32) {
  ProgramBuilder program;
  auto type = program.I32Type();
  auto func = program.CreatePublicFunction(type, {type}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto ptr = program.Alloca(type);
  program.StoreI32(ptr, args[0]);
  program.Return(program.LoadI32(ptr));

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<int32_t(int32_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    int32_t c = distrib(gen);
    EXPECT_EQ(c, compute(c));
  }
}

TEST_P(BackendTest, ALLOCAI64) {
  ProgramBuilder program;
  auto type = program.I64Type();
  auto func = program.CreatePublicFunction(type, {type}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto ptr = program.Alloca(type);
  program.StoreI64(ptr, args[0]);
  program.Return(program.LoadI64(ptr));

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<int64_t(int64_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    int64_t c = distrib(gen);
    EXPECT_EQ(c, compute(c));
  }
}

TEST_P(BackendTest, ALLOCAF64) {
  ProgramBuilder program;
  auto type = program.F64Type();
  auto func = program.CreatePublicFunction(type, {type}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto ptr = program.Alloca(type);
  program.StoreF64(ptr, args[0]);
  program.Return(program.LoadF64(ptr));

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<double(double)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    double c = distrib(gen);
    EXPECT_EQ(c, compute(c));
  }
}

TEST_P(BackendTest, ALLOCAStruct) {
  struct Test {
    int8_t x1;
    int16_t x2;
    int64_t x3;
  };

  ProgramBuilder program;
  auto test_type = program.StructType(
      {program.I8Type(), program.I16Type(), program.I64Type()});

  auto handler_type = program.FunctionType(program.VoidType(),
                                           {program.PointerType(test_type)});
  auto handler_pointer_type = program.PointerType(handler_type);

  auto func = program.CreatePublicFunction(program.VoidType(),
                                           {handler_pointer_type}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto ptr = program.Alloca(test_type);
  program.StoreI8(program.ConstGEP(test_type, ptr, {0, 0}), program.ConstI8(0));
  program.StoreI16(program.ConstGEP(test_type, ptr, {0, 1}),
                   program.ConstI16(1000));
  program.StoreI64(program.ConstGEP(test_type, ptr, {0, 2}),
                   program.ConstI64(2));
  program.Call(args[0], {ptr});
  program.Return();

  auto backend = Compile(GetParam(), program);

  using handler_fn = std::add_pointer<void(Test*)>::type;
  using compute_fn = std::add_pointer<void(handler_fn)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  compute([](Test* t) {
    EXPECT_EQ(0, t->x1);
    EXPECT_EQ(1000, t->x2);
    EXPECT_EQ(2, t->x3);
  });
}

TEST_P(BackendTest, PTR_LOAD) {
  ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.PointerType(program.I64Type()),
      {program.PointerType(program.PointerType(program.I64Type()))}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.LoadPtr(args[0]));

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<int64_t*(int64_t**)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  int64_t value = 0xDEADBEEF;
  int64_t* ptr = reinterpret_cast<int64_t*>(value);
  EXPECT_EQ(ptr, compute(&ptr));
}

TEST_P(BackendTest, PTR_LOADGlobal) {
  ProgramBuilder program;
  auto global =
      program.Global(true, false, program.PointerType(program.I64Type()),
                     program.NullPtr(program.PointerType(program.I64Type())));
  program.CreatePublicFunction(program.PointerType(program.I64Type()), {},
                               "compute");
  program.Return(program.LoadPtr(global));

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<int64_t*()>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  EXPECT_EQ(nullptr, compute());
}

TEST_P(BackendTest, PTR_LOADStruct) {
  struct Test {
    int64_t* x;
  };

  ProgramBuilder program;
  auto st = program.StructType({program.PointerType(program.I64Type())});
  auto func =
      program.CreatePublicFunction(program.PointerType(program.I64Type()),
                                   {program.PointerType(st)}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.LoadPtr(program.ConstGEP(st, args[0], {0, 0})));

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<int64_t*(Test*)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  Test t{.x = reinterpret_cast<int64_t*>(0xDEADBEEF)};
  EXPECT_EQ(t.x, compute(&t));
}

TEST_P(BackendTest, PTR_STORE) {
  ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.VoidType(),
      {program.PointerType(program.PointerType(program.I64Type())),
       program.PointerType(program.I64Type())},
      "compute");
  auto args = program.GetFunctionArguments(func);
  program.StorePtr(args[0], args[1]);
  program.Return();

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<void(int64_t**, int64_t*)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  int64_t value = 0xDEADBEEF;
  int64_t* ptr = reinterpret_cast<int64_t*>(value);
  int64_t* loc;
  compute(&loc, ptr);
  EXPECT_EQ(ptr, loc);
}

TEST_P(BackendTest, PTR_STORENullptr) {
  ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.VoidType(),
      {program.PointerType(program.PointerType(program.I64Type()))}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.StorePtr(args[0],
                   program.NullPtr(program.PointerType(program.I64Type())));
  program.Return();

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<void(int64_t**)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  int64_t value = 0xDEADBEEF;
  int64_t* ptr = reinterpret_cast<int64_t*>(value);
  compute(&ptr);
  EXPECT_EQ(nullptr, ptr);
}

TEST_P(BackendTest, PTR_STOREGlobal) {
  ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I64Type(),
      {program.PointerType(program.PointerType(program.I64Type()))}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto global =
      program.Global(true, false, program.I64Type(), program.ConstI64(-1));
  program.StorePtr(args[0], global);
  auto ptr = program.LoadPtr(args[0]);
  program.Return(program.LoadI64(ptr));

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<int64_t(int64_t**)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  int64_t* loc = nullptr;
  EXPECT_EQ(-1, compute(&loc));
}

TEST_P(BackendTest, PTR_STOREGep) {
  struct Test {
    int32_t a;
    int64_t b;
  };

  ProgramBuilder program;
  auto st = program.StructType({program.I32Type(), program.I64Type()});
  auto func = program.CreatePublicFunction(
      program.VoidType(),
      {program.PointerType(program.PointerType(program.I64Type())),
       program.PointerType(st)},
      "compute");
  auto args = program.GetFunctionArguments(func);
  program.StorePtr(args[0], program.ConstGEP(st, args[1], {0, 1}));
  program.Return();

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<void(int64_t**, Test*)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  Test x{.a = 100, .b = -1000};
  int64_t* loc = nullptr;
  compute(&loc, &x);
  EXPECT_EQ(&x.b, loc);
}

TEST_P(BackendTest, PTR_CMP_NULLPTR_Gep) {
  struct Test {
    int64_t* a;
  };

  ProgramBuilder program;
  auto st = program.StructType({program.PointerType(program.I64Type())});
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.PointerType(st)}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto v = program.LoadPtr(program.ConstGEP(st, args[0], {0, 0}));
  program.Return(program.IsNullPtr(v));

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<bool(Test*)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  Test x{.a = nullptr};
  EXPECT_TRUE(compute(&x));
}

TEST_P(BackendTest, PTR_STOREGlobalDest) {
  ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.PointerType(program.I64Type()),
      {program.PointerType(program.I64Type())}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto global =
      program.Global(false, true, program.PointerType(program.I64Type()),
                     program.NullPtr(program.PointerType(program.I64Type())));
  program.StorePtr(
      global,
      program.PointerCast(args[0], program.PointerType(program.I64Type())));
  program.Return(program.LoadPtr(global));

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<int64_t*(int64_t*)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  int64_t value = 0xDEADBEEF;
  int64_t* ptr = reinterpret_cast<int64_t*>(value);
  EXPECT_EQ(ptr, compute(ptr));
}

TEST_P(BackendTest, PTR_STOREGepDest) {
  struct Test {
    int64_t* x;
  };

  ProgramBuilder program;
  auto st = program.StructType({program.PointerType(program.I64Type())});
  auto func = program.CreatePublicFunction(
      program.VoidType(),
      {program.PointerType(st), program.PointerType(program.I64Type())},
      "compute");
  auto args = program.GetFunctionArguments(func);
  program.StorePtr(program.ConstGEP(st, args[0], {0, 0}), args[1]);
  program.Return();

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<void(Test*, int64_t*)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  Test t{.x = nullptr};
  int64_t value = 0xDEADBEEF;
  int64_t* ptr = reinterpret_cast<int64_t*>(value);
  compute(&t, ptr);
  EXPECT_EQ(ptr, t.x);
}

TEST_P(BackendTest, PHIPTR) {
  int64_t c1 = 0xDEADBEEF;
  int64_t* p1 = reinterpret_cast<int64_t*>(c1);
  int64_t c2 = 0xBEAFDEAD;
  int64_t* p2 = reinterpret_cast<int64_t*>(c2);

  ProgramBuilder program;
  auto type = program.PointerType(program.I64Type());
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

  auto backend = Compile(GetParam(), program);

  using compute_fn =
      std::add_pointer<int64_t*(int8_t, int64_t*, int64_t*)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  EXPECT_EQ(p1, compute(1, p1, p2));
  EXPECT_EQ(p2, compute(0, p1, p2));
}

TEST_P(BackendTest, I1_CONST) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(0, 1);
  for (int i = 0; i < 10; i++) {
    bool c = distrib(gen);

    ProgramBuilder program;
    program.CreatePublicFunction(program.I1Type(), {}, "compute");
    program.Return(program.ConstI1(c));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<bool()>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(c, compute());
  }
}

TEST_P(BackendTest, I1_AND) {
  ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.I1Type(), program.I1Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.AndI1(args[0], args[1]));

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<bool(bool, bool)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  EXPECT_EQ(true, compute(true, true));
  EXPECT_EQ(false, compute(true, false));
  EXPECT_EQ(false, compute(false, true));
  EXPECT_EQ(false, compute(false, false));
}

TEST_P(BackendTest, I1_ANDConstArg0) {
  for (auto c : {true, false}) {
    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I1Type(),
                                             {program.I1Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.AndI1(program.ConstI1(c), args[0]));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<bool(bool)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(c && true, compute(true));
    EXPECT_EQ(c && false, compute(false));
  }
}

TEST_P(BackendTest, I1_ANDConstArg1) {
  for (auto c : {true, false}) {
    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I1Type(),
                                             {program.I1Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.AndI1(args[0], program.ConstI1(c)));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<bool(bool)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(true && c, compute(true));
    EXPECT_EQ(false && c, compute(false));
  }
}

TEST_P(BackendTest, I1_OR) {
  ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.I1Type(), program.I1Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.OrI1(args[0], args[1]));

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<bool(bool, bool)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  EXPECT_EQ(true, compute(true, true));
  EXPECT_EQ(true, compute(true, false));
  EXPECT_EQ(true, compute(false, true));
  EXPECT_EQ(false, compute(false, false));
}

TEST_P(BackendTest, I1_ORConstArg0) {
  for (auto c : {true, false}) {
    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I1Type(),
                                             {program.I1Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.OrI1(program.ConstI1(c), args[0]));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<bool(bool)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(c || true, compute(true));
    EXPECT_EQ(c || false, compute(false));
  }
}

TEST_P(BackendTest, I1_ORConstArg1) {
  for (auto c : {true, false}) {
    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I1Type(),
                                             {program.I1Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.OrI1(args[0], program.ConstI1(c)));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<bool(bool)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(true || c, compute(true));
    EXPECT_EQ(false || c, compute(false));
  }
}

TEST_P(BackendTest, I1_LNOT) {
  ProgramBuilder program;
  auto func = program.CreatePublicFunction(program.I1Type(), {program.I1Type()},
                                           "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.LNotI1(args[0]));

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<bool(bool)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  EXPECT_EQ(false, compute(true));
  EXPECT_EQ(true, compute(false));
}

TEST_P(BackendTest, I1_LNOTConst) {
  for (auto c : {true, false}) {
    ProgramBuilder program;
    program.CreatePublicFunction(program.I1Type(), {}, "compute");
    program.Return(program.LNotI1(program.ConstI1(c)));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<bool()>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(!c, compute());
  }
}

TEST_P(BackendTest, I1_CMP_XXReturn) {
  for (auto cmp_type : {CompType::EQ, CompType::NE}) {
    int8_t c1 = 0;
    int8_t c2 = 1;

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(
        program.I1Type(), {program.I1Type(), program.I1Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.CmpI1(cmp_type, args[0], args[1]));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int8_t(int8_t, int8_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

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

TEST_P(BackendTest, I1_CMP_XXBranch) {
  for (auto cmp_type : {CompType::EQ, CompType::NE}) {
    int8_t c1 = 0;
    int8_t c2 = 1;

    ProgramBuilder program;
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

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int8_t(int8_t, int8_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

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

TEST_P(BackendTest, I1_CMP_XXReturnConstArg0) {
  for (auto cmp_type : {CompType::EQ, CompType::NE}) {
    int8_t c1 = 0;
    int8_t c2 = 1;

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I1Type(),
                                             {program.I1Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.CmpI1(cmp_type, program.ConstI1(c1), args[0]));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int8_t(int8_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

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

TEST_P(BackendTest, I1_CMP_XXReturnConstArg1) {
  for (auto cmp_type : {CompType::EQ, CompType::NE}) {
    int8_t c1 = 0;
    int8_t c2 = 1;

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I1Type(),
                                             {program.I1Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.CmpI1(cmp_type, args[0], program.ConstI1(c2)));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int8_t(int8_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

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

TEST_P(BackendTest, I1_ZEXT_I64) {
  for (auto c : {true, false}) {
    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I64Type(),
                                             {program.I1Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.I64ZextI1(args[0]));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int64_t(bool)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int64_t zexted = c ? 1 : 0;
    EXPECT_EQ(zexted, compute(c));
  }
}

TEST_P(BackendTest, I1_ZEXT_I64Const) {
  for (auto c : {true, false}) {
    ProgramBuilder program;
    program.CreatePublicFunction(program.I64Type(), {}, "compute");
    program.Return(program.I64ZextI1(program.ConstI1(c)));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int64_t()>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int64_t zexted = c ? 1 : 0;
    EXPECT_EQ(zexted, compute());
  }
}

TEST_P(BackendTest, I1_ZEXT_I8) {
  for (auto c : {true, false}) {
    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I8Type(),
                                             {program.I1Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.I8ZextI1(args[0]));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int8_t(bool)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(c, compute(c));
  }
}

TEST_P(BackendTest, I1_ZEXT_I8Const) {
  for (auto c : {true, false}) {
    ProgramBuilder program;
    program.CreatePublicFunction(program.I8Type(), {}, "compute");
    program.Return(program.I8ZextI1(program.ConstI1(c)));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int8_t()>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(c, compute());
  }
}

TEST_P(BackendTest, PHII1) {
  bool c1 = false;
  bool c2 = true;

  ProgramBuilder program;
  auto type = program.I1Type();
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

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<int8_t(int8_t, int8_t, int8_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  EXPECT_EQ(c1, compute(1, c1, c2));
  EXPECT_EQ(c2, compute(0, c1, c2));
}

TEST_P(BackendTest, PHII1ConstArg0) {
  bool c1 = false;
  bool c2 = true;

  ProgramBuilder program;
  auto type = program.I1Type();
  auto func =
      program.CreatePublicFunction(type, {program.I1Type(), type}, "compute");
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

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<int8_t(int8_t, int8_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  EXPECT_EQ(c1, compute(1, c2));
  EXPECT_EQ(c2, compute(0, c2));
}

TEST_P(BackendTest, PHII1ConstArg1) {
  bool c1 = false;
  bool c2 = true;

  ProgramBuilder program;
  auto type = program.I1Type();
  auto func =
      program.CreatePublicFunction(type, {program.I1Type(), type}, "compute");
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

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<int8_t(int8_t, int8_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  EXPECT_EQ(c1, compute(1, c1));
  EXPECT_EQ(c2, compute(0, c1));
}

TEST_P(BackendTest, I8_ADD) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);

  ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I8Type(), {program.I8Type(), program.I8Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.AddI8(args[0], args[1]);
  program.Return(sum);

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<int8_t(int8_t, int8_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  for (int i = 0; i < 10; i++) {
    int8_t a0 = distrib(gen);
    int8_t a1 = distrib(gen);
    int8_t res = a0 + a1;
    EXPECT_EQ(res, compute(a0, a1));
    EXPECT_EQ(res, compute(a1, a0));
  }
}

TEST_P(BackendTest, I8_ADDConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);

  for (int i = 0; i < 10; i++) {
    int8_t a0 = distrib(gen);
    int8_t a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I8Type(),
                                             {program.I8Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.AddI8(program.ConstI8(a0), args[0]);
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int8_t(int8_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int8_t res = a0 + a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST_P(BackendTest, I8_ADDConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);

  for (int i = 0; i < 10; i++) {
    int8_t a0 = distrib(gen);
    int8_t a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I8Type(),
                                             {program.I8Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.AddI8(args[0], program.ConstI8(a1));
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int8_t(int8_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int8_t res = a0 + a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST_P(BackendTest, I8_SUB) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);

  ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I8Type(), {program.I8Type(), program.I8Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.SubI8(args[0], args[1]);
  program.Return(sum);

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<int8_t(int8_t, int8_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  for (int i = 0; i < 10; i++) {
    int8_t a0 = distrib(gen);
    int8_t a1 = distrib(gen);
    int8_t res = a0 - a1;
    EXPECT_EQ(res, compute(a0, a1));
  }
}

TEST_P(BackendTest, I8_SubConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);

  for (int i = 0; i < 10; i++) {
    int8_t a0 = distrib(gen);
    int8_t a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I8Type(),
                                             {program.I8Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.SubI8(program.ConstI8(a0), args[0]);
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int8_t(int8_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int8_t res = a0 - a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST_P(BackendTest, I8_SubConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);

  for (int i = 0; i < 10; i++) {
    int8_t a0 = distrib(gen);
    int8_t a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I8Type(),
                                             {program.I8Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.SubI8(args[0], program.ConstI8(a1));
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int8_t(int8_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int8_t res = a0 - a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST_P(BackendTest, I8_MUL) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);

  ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I8Type(), {program.I8Type(), program.I8Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.MulI8(args[0], args[1]);
  program.Return(sum);

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<int8_t(int8_t, int8_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  for (int i = 0; i < 10; i++) {
    int8_t a0 = distrib(gen);
    int8_t a1 = distrib(gen);
    int8_t res = a0 * a1;
    EXPECT_EQ(res, compute(a0, a1));
  }
}

TEST_P(BackendTest, I8_MULConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);

  for (int i = 0; i < 10; i++) {
    int8_t a0 = distrib(gen);
    int8_t a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I8Type(),
                                             {program.I8Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.MulI8(program.ConstI8(a0), args[0]);
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int8_t(int8_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int8_t res = a0 * a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST_P(BackendTest, I8_MULConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);

  for (int i = 0; i < 10; i++) {
    int8_t a0 = distrib(gen);
    int8_t a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I8Type(),
                                             {program.I8Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.MulI8(args[0], program.ConstI8(a1));
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int8_t(int8_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int8_t res = a0 * a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST_P(BackendTest, I8_CONST) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    int8_t c = distrib(gen);

    ProgramBuilder program;
    program.CreatePublicFunction(program.I8Type(), {}, "compute");
    program.Return(program.ConstI8(c));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int8_t()>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(c, compute());
  }
}

TEST_P(BackendTest, I8_ZEXT_I64) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I64Type(),
                                             {program.I8Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.I64ZextI8(args[0]));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int64_t(int8_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int8_t c = distrib(gen);
    int64_t zexted = int64_t(c) & 0xFF;
    EXPECT_EQ(zexted, compute(c));
  }
}

TEST_P(BackendTest, I8_ZEXT_I64Const) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    int8_t c = distrib(gen);

    ProgramBuilder program;
    program.CreatePublicFunction(program.I64Type(), {program.I8Type()},
                                 "compute");
    program.Return(program.I64ZextI8(program.ConstI8(c)));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int64_t(int8_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int64_t zexted = int64_t(c) & 0xFF;
    EXPECT_EQ(zexted, compute(c));
  }
}

TEST_P(BackendTest, I8_CONV_F64) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.F64Type(),
                                             {program.I8Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.F64ConvI8(args[0]));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<double(int8_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int8_t c = distrib(gen);
    double conv = c;
    EXPECT_EQ(conv, compute(c));
  }
}

TEST_P(BackendTest, I8_CONV_F64Const) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    int8_t c = distrib(gen);

    ProgramBuilder program;
    program.CreatePublicFunction(program.F64Type(), {program.I8Type()},
                                 "compute");
    program.Return(program.F64ConvI8(program.ConstI8(c)));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<double(int8_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    double conv = c;
    EXPECT_EQ(conv, compute(c));
  }
}

TEST_P(BackendTest, I8_CMP_XXReturn) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type : {CompType::EQ, CompType::NE, CompType::LT,
                          CompType::LE, CompType::GT, CompType::GE}) {
      int8_t c1 = distrib(gen);
      int8_t c2 = distrib(gen);

      ProgramBuilder program;
      auto func = program.CreatePublicFunction(
          program.I1Type(), {program.I8Type(), program.I8Type()}, "compute");
      auto args = program.GetFunctionArguments(func);
      program.Return(program.CmpI8(cmp_type, args[0], args[1]));

      auto backend = Compile(GetParam(), program);

      using compute_fn = std::add_pointer<int8_t(int8_t, int8_t)>::type;
      auto compute =
          reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

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

TEST_P(BackendTest, I8_CMP_XXBranch) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type : {CompType::EQ, CompType::NE, CompType::LT,
                          CompType::LE, CompType::GT, CompType::GE}) {
      int8_t c1 = distrib(gen);
      int8_t c2 = distrib(gen);

      ProgramBuilder program;
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

      auto backend = Compile(GetParam(), program);

      using compute_fn = std::add_pointer<int8_t(int8_t, int8_t)>::type;
      auto compute =
          reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

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

TEST_P(BackendTest, I8_CMP_XXConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type : {CompType::EQ, CompType::NE, CompType::LT,
                          CompType::LE, CompType::GT, CompType::GE}) {
      int8_t c1 = distrib(gen);
      int8_t c2 = distrib(gen);

      ProgramBuilder program;
      auto func = program.CreatePublicFunction(program.I1Type(),
                                               {program.I8Type()}, "compute");
      auto args = program.GetFunctionArguments(func);
      program.Return(program.CmpI8(cmp_type, program.ConstI8(c1), args[0]));

      auto backend = Compile(GetParam(), program);

      using compute_fn = std::add_pointer<int8_t(int8_t)>::type;
      auto compute =
          reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

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

TEST_P(BackendTest, I8_CMP_XXConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type : {CompType::EQ, CompType::NE, CompType::LT,
                          CompType::LE, CompType::GT, CompType::GE}) {
      int8_t c1 = distrib(gen);
      int8_t c2 = distrib(gen);

      ProgramBuilder program;
      auto func = program.CreatePublicFunction(program.I1Type(),
                                               {program.I8Type()}, "compute");
      auto args = program.GetFunctionArguments(func);
      program.Return(program.CmpI8(cmp_type, args[0], program.ConstI8(c2)));

      auto backend = Compile(GetParam(), program);

      using compute_fn = std::add_pointer<int8_t(int8_t)>::type;
      auto compute =
          reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

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

TEST_P(BackendTest, I8_LOAD) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    int8_t loc = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(
        program.I8Type(), {program.PointerType(program.I8Type())}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.LoadI8(args[0]));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int8_t(int8_t*)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(loc, compute(&loc));
  }
}

TEST_P(BackendTest, I8_LOADGlobal) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    int8_t c = distrib(gen);

    ProgramBuilder program;
    auto global =
        program.Global(true, false, program.I8Type(), program.ConstI8(c));
    program.CreatePublicFunction(program.I8Type(), {}, "compute");
    program.Return(program.LoadI8(global));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int8_t()>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(c, compute());
  }
}

TEST_P(BackendTest, I8_LOADStruct) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    int8_t c = distrib(gen);

    struct Test {
      int8_t x;
    };

    ProgramBuilder program;
    auto st = program.StructType({program.I8Type()});
    auto func = program.CreatePublicFunction(
        program.I8Type(), {program.PointerType(st)}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.LoadI8(program.ConstGEP(st, args[0], {0, 0})));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int8_t(Test*)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    Test t{.x = c};
    EXPECT_EQ(c, compute(&t));
  }
}

TEST_P(BackendTest, I8_STORE) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    int8_t c = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(
        program.VoidType(),
        {program.PointerType(program.I8Type()), program.I8Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreI8(args[0], args[1]);
    program.Return();

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<void(int8_t*, int8_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int8_t loc;
    compute(&loc, c);
    EXPECT_EQ(loc, c);
  }
}

TEST_P(BackendTest, I8_STOREConst) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    int8_t c = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.PointerType(program.I8Type())}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreI8(args[0], program.ConstI8(c));
    program.Return();

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<void(int8_t*)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int8_t loc;
    compute(&loc);
    EXPECT_EQ(loc, c);
  }
}

TEST_P(BackendTest, I8_STOREGlobal) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    int8_t c = distrib(gen);

    ProgramBuilder program;
    auto global =
        program.Global(true, false, program.I8Type(), program.ConstI8(c));
    auto func = program.CreatePublicFunction(
        program.PointerType(program.I8Type()), {program.I8Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreI8(global, args[0]);
    program.Return(global);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int8_t*(int8_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    auto loc = compute(c);
    EXPECT_EQ(*loc, c);
  }
}

TEST_P(BackendTest, I8_STOREStruct) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    int8_t c = distrib(gen);

    struct Test {
      int8_t x;
    };

    ProgramBuilder program;
    auto st = program.StructType({program.I8Type()});
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.PointerType(st), program.I8Type()},
        "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreI8(program.ConstGEP(st, args[0], {0, 0}), args[1]);
    program.Return();

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<void(Test*, int8_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    Test t;
    compute(&t, c);
    EXPECT_EQ(t.x, c);
  }
}

TEST_P(BackendTest, PHII8) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    int8_t c1 = distrib(gen);
    int8_t c2 = distrib(gen);

    ProgramBuilder program;
    auto type = program.I8Type();
    auto func = program.CreatePublicFunction(
        type, {program.I1Type(), type, type}, "compute");
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

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int8_t(int8_t, int8_t, int8_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(c1, compute(1, c1, c2));
    EXPECT_EQ(c2, compute(0, c1, c2));
  }
}

TEST_P(BackendTest, PHII8ConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    int8_t c1 = distrib(gen);
    int8_t c2 = distrib(gen);

    ProgramBuilder program;
    auto type = program.I8Type();
    auto func =
        program.CreatePublicFunction(type, {program.I1Type(), type}, "compute");
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

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int8_t(int8_t, int8_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(c1, compute(1, c2));
    EXPECT_EQ(c2, compute(0, c2));
  }
}

TEST_P(BackendTest, PHII8ConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int8_t> distrib(INT8_MIN, INT8_MAX);
  for (int i = 0; i < 10; i++) {
    int8_t c1 = distrib(gen);
    int8_t c2 = distrib(gen);

    ProgramBuilder program;
    auto type = program.I8Type();
    auto func =
        program.CreatePublicFunction(type, {program.I1Type(), type}, "compute");
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

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int8_t(int8_t, int8_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(c1, compute(1, c1));
    EXPECT_EQ(c2, compute(0, c1));
  }
}

TEST_P(BackendTest, I16_ADD) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);

  ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I16Type(), {program.I16Type(), program.I16Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.AddI16(args[0], args[1]);
  program.Return(sum);

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<int16_t(int16_t, int16_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  for (int i = 0; i < 10; i++) {
    int16_t a0 = distrib(gen);
    int16_t a1 = distrib(gen);
    int16_t res = a0 + a1;
    EXPECT_EQ(res, compute(a0, a1));
  }
}

TEST_P(BackendTest, I16_ADDConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);

  for (int i = 0; i < 10; i++) {
    int16_t a0 = distrib(gen);
    int16_t a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I16Type(),
                                             {program.I16Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.AddI16(program.ConstI16(a0), args[0]);
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int16_t(int16_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int16_t res = a0 + a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST_P(BackendTest, I16_ADDConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);

  for (int i = 0; i < 10; i++) {
    int16_t a0 = distrib(gen);
    int16_t a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I16Type(),
                                             {program.I16Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.AddI16(args[0], program.ConstI16(a1));
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int16_t(int16_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int16_t res = a0 + a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST_P(BackendTest, I16_SUB) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);

  ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I16Type(), {program.I16Type(), program.I16Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.SubI16(args[0], args[1]);
  program.Return(sum);

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<int16_t(int16_t, int16_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  for (int i = 0; i < 10; i++) {
    int16_t a0 = distrib(gen);
    int16_t a1 = distrib(gen);
    int16_t res = a0 - a1;
    EXPECT_EQ(res, compute(a0, a1));
  }
}

TEST_P(BackendTest, I16_SubConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);

  for (int i = 0; i < 10; i++) {
    int16_t a0 = distrib(gen);
    int16_t a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I16Type(),
                                             {program.I16Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.SubI16(program.ConstI16(a0), args[0]);
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int16_t(int16_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int16_t res = a0 - a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST_P(BackendTest, I16_SubConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);

  for (int i = 0; i < 10; i++) {
    int16_t a0 = distrib(gen);
    int16_t a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I16Type(),
                                             {program.I16Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.SubI16(args[0], program.ConstI16(a1));
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int16_t(int16_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int16_t res = a0 - a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST_P(BackendTest, I16_MUL) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);

  ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I16Type(), {program.I16Type(), program.I16Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.MulI16(args[0], args[1]);
  program.Return(sum);

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<int16_t(int16_t, int16_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  for (int i = 0; i < 10; i++) {
    int16_t a0 = distrib(gen);
    int16_t a1 = distrib(gen);
    int16_t res = a0 * a1;
    EXPECT_EQ(res, compute(a0, a1));
  }
}

TEST_P(BackendTest, I16_MULConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);

  for (int i = 0; i < 10; i++) {
    int16_t a0 = distrib(gen);
    int16_t a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I16Type(),
                                             {program.I16Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.MulI16(program.ConstI16(a0), args[0]);
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int16_t(int16_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int16_t res = a0 * a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST_P(BackendTest, I16_MULConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);

  for (int i = 0; i < 10; i++) {
    int16_t a0 = distrib(gen);
    int16_t a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I16Type(),
                                             {program.I16Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.MulI16(args[0], program.ConstI16(a1));
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int16_t(int16_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int16_t res = a0 * a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST_P(BackendTest, I16_CONST) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    int16_t c = distrib(gen);

    ProgramBuilder program;
    program.CreatePublicFunction(program.I16Type(), {}, "compute");
    program.Return(program.ConstI16(c));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int16_t()>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(c, compute());
  }
}

TEST_P(BackendTest, I16_ZEXT_I64) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I64Type(),
                                             {program.I16Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.I64ZextI16(args[0]));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int64_t(int16_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int16_t c = distrib(gen);
    int64_t zexted = int64_t(c) & 0xFFFF;
    EXPECT_EQ(zexted, compute(c));
  }
}

TEST_P(BackendTest, I16_ZEXT_I64Const) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    int16_t c = distrib(gen);

    ProgramBuilder program;
    program.CreatePublicFunction(program.I64Type(), {program.I16Type()},
                                 "compute");
    program.Return(program.I64ZextI16(program.ConstI16(c)));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int64_t(int16_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int64_t zexted = int64_t(c) & 0xFFFF;
    EXPECT_EQ(zexted, compute(c));
  }
}

TEST_P(BackendTest, I16_CONV_F64) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.F64Type(),
                                             {program.I16Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.F64ConvI16(args[0]));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<double(int16_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int16_t c = distrib(gen);
    double conv = c;
    EXPECT_EQ(conv, compute(c));
  }
}

TEST_P(BackendTest, I16_CONV_F64Const) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    int16_t c = distrib(gen);

    ProgramBuilder program;
    program.CreatePublicFunction(program.F64Type(), {program.I16Type()},
                                 "compute");
    program.Return(program.F64ConvI16(program.ConstI16(c)));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<double(int16_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    double conv = c;
    EXPECT_EQ(conv, compute(c));
  }
}

TEST_P(BackendTest, I16_CMP_XXReturn) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type : {CompType::EQ, CompType::NE, CompType::LT,
                          CompType::LE, CompType::GT, CompType::GE}) {
      int16_t c1 = distrib(gen);
      int16_t c2 = distrib(gen);

      ProgramBuilder program;
      auto func = program.CreatePublicFunction(
          program.I1Type(), {program.I16Type(), program.I16Type()}, "compute");
      auto args = program.GetFunctionArguments(func);
      program.Return(program.CmpI16(cmp_type, args[0], args[1]));

      auto backend = Compile(GetParam(), program);

      using compute_fn = std::add_pointer<int8_t(int16_t, int16_t)>::type;
      auto compute =
          reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

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

TEST_P(BackendTest, I16_CMP_XXBranch) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type : {CompType::EQ, CompType::NE, CompType::LT,
                          CompType::LE, CompType::GT, CompType::GE}) {
      int16_t c1 = distrib(gen);
      int16_t c2 = distrib(gen);

      ProgramBuilder program;
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

      auto backend = Compile(GetParam(), program);

      using compute_fn = std::add_pointer<int8_t(int16_t, int16_t)>::type;
      auto compute =
          reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

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

TEST_P(BackendTest, I16_CMP_XXConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type : {CompType::EQ, CompType::NE, CompType::LT,
                          CompType::LE, CompType::GT, CompType::GE}) {
      int16_t c1 = distrib(gen);
      int16_t c2 = distrib(gen);

      ProgramBuilder program;
      auto func = program.CreatePublicFunction(program.I1Type(),
                                               {program.I16Type()}, "compute");
      auto args = program.GetFunctionArguments(func);
      program.Return(program.CmpI16(cmp_type, program.ConstI16(c1), args[0]));

      auto backend = Compile(GetParam(), program);

      using compute_fn = std::add_pointer<int8_t(int16_t)>::type;
      auto compute =
          reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

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

TEST_P(BackendTest, I16_CMP_XXConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type : {CompType::EQ, CompType::NE, CompType::LT,
                          CompType::LE, CompType::GT, CompType::GE}) {
      int16_t c1 = distrib(gen);
      int16_t c2 = distrib(gen);

      ProgramBuilder program;
      auto func = program.CreatePublicFunction(program.I1Type(),
                                               {program.I16Type()}, "compute");
      auto args = program.GetFunctionArguments(func);
      program.Return(program.CmpI16(cmp_type, args[0], program.ConstI16(c2)));

      auto backend = Compile(GetParam(), program);

      using compute_fn = std::add_pointer<int8_t(int16_t)>::type;
      auto compute =
          reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

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

TEST_P(BackendTest, I16_LOAD) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    int16_t loc = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(
        program.I16Type(), {program.PointerType(program.I16Type())}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.LoadI16(args[0]));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int16_t(int16_t*)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(loc, compute(&loc));
  }
}

TEST_P(BackendTest, I16_LOADGlobal) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    int16_t c = distrib(gen);

    ProgramBuilder program;
    auto global =
        program.Global(true, false, program.I16Type(), program.ConstI16(c));
    program.CreatePublicFunction(program.I16Type(), {}, "compute");
    program.Return(program.LoadI16(global));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int16_t()>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(c, compute());
  }
}

TEST_P(BackendTest, I16_LOADStruct) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    int16_t c = distrib(gen);

    struct Test {
      int16_t x;
    };

    ProgramBuilder program;
    auto st = program.StructType({program.I16Type()});
    auto func = program.CreatePublicFunction(
        program.I16Type(), {program.PointerType(st)}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.LoadI16(program.ConstGEP(st, args[0], {0, 0})));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int16_t(Test*)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    Test t{.x = c};
    EXPECT_EQ(c, compute(&t));
  }
}

TEST_P(BackendTest, I16_STORE) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    int16_t c = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(
        program.VoidType(),
        {program.PointerType(program.I16Type()), program.I16Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreI16(args[0], args[1]);
    program.Return();

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<void(int16_t*, int16_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int16_t loc;
    compute(&loc, c);
    EXPECT_EQ(loc, c);
  }
}

TEST_P(BackendTest, I16_STOREConst) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    int16_t c = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.PointerType(program.I16Type())},
        "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreI16(args[0], program.ConstI16(c));
    program.Return();

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<void(int16_t*)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int16_t loc;
    compute(&loc);
    EXPECT_EQ(loc, c);
  }
}

TEST_P(BackendTest, I16_STOREGlobal) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    int16_t c = distrib(gen);

    ProgramBuilder program;
    auto global =
        program.Global(true, false, program.I16Type(), program.ConstI16(c));
    auto func = program.CreatePublicFunction(
        program.PointerType(program.I16Type()), {program.I16Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreI16(global, args[0]);
    program.Return(global);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int16_t*(int16_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    auto loc = compute(c);
    EXPECT_EQ(*loc, c);
  }
}

TEST_P(BackendTest, I16_STOREStruct) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    int16_t c = distrib(gen);

    struct Test {
      int16_t x;
    };

    ProgramBuilder program;
    auto st = program.StructType({program.I16Type()});
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.PointerType(st), program.I16Type()},
        "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreI16(program.ConstGEP(st, args[0], {0, 0}), args[1]);
    program.Return();

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<void(Test*, int16_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    Test t;
    compute(&t, c);
    EXPECT_EQ(t.x, c);
  }
}

TEST_P(BackendTest, PHII16) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    int16_t c1 = distrib(gen);
    int16_t c2 = distrib(gen);

    ProgramBuilder program;
    auto type = program.I16Type();
    auto func = program.CreatePublicFunction(
        type, {program.I1Type(), type, type}, "compute");
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

    auto backend = Compile(GetParam(), program);

    using compute_fn =
        std::add_pointer<int16_t(int8_t, int16_t, int16_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(c1, compute(1, c1, c2));
    EXPECT_EQ(c2, compute(0, c1, c2));
  }
}

TEST_P(BackendTest, PHII16ConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    int16_t c1 = distrib(gen);
    int16_t c2 = distrib(gen);

    ProgramBuilder program;
    auto type = program.I16Type();
    auto func =
        program.CreatePublicFunction(type, {program.I1Type(), type}, "compute");
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

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int16_t(int8_t, int16_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(c1, compute(1, c2));
    EXPECT_EQ(c2, compute(0, c2));
  }
}

TEST_P(BackendTest, PHII16ConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> distrib(INT16_MIN, INT16_MAX);
  for (int i = 0; i < 10; i++) {
    int16_t c1 = distrib(gen);
    int16_t c2 = distrib(gen);

    ProgramBuilder program;
    auto type = program.I16Type();
    auto func =
        program.CreatePublicFunction(type, {program.I1Type(), type}, "compute");
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

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int16_t(int8_t, int16_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(c1, compute(1, c1));
    EXPECT_EQ(c2, compute(0, c1));
  }
}

TEST_P(BackendTest, I32_ADD) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);

  ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I32Type(), {program.I32Type(), program.I32Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.AddI32(args[0], args[1]);
  program.Return(sum);

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<int32_t(int32_t, int32_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  for (int i = 0; i < 10; i++) {
    int32_t a0 = distrib(gen);
    int32_t a1 = distrib(gen);
    int32_t res = a0 + a1;
    EXPECT_EQ(res, compute(a0, a1));
  }
}

TEST_P(BackendTest, I32_ADDConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);

  for (int i = 0; i < 10; i++) {
    int32_t a0 = distrib(gen);
    int32_t a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I32Type(),
                                             {program.I32Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.AddI32(program.ConstI32(a0), args[0]);
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int32_t(int32_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int32_t res = a0 + a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST_P(BackendTest, I32_ADDConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);

  for (int i = 0; i < 10; i++) {
    int32_t a0 = distrib(gen);
    int32_t a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I32Type(),
                                             {program.I32Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.AddI32(args[0], program.ConstI32(a1));
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int32_t(int32_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int32_t res = a0 + a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST_P(BackendTest, I32_SUB) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);

  ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I32Type(), {program.I32Type(), program.I32Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.SubI32(args[0], args[1]);
  program.Return(sum);

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<int32_t(int32_t, int32_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  for (int i = 0; i < 10; i++) {
    int32_t a0 = distrib(gen);
    int32_t a1 = distrib(gen);
    int32_t res = a0 - a1;
    EXPECT_EQ(res, compute(a0, a1));
  }
}

TEST_P(BackendTest, I32_SubConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);

  for (int i = 0; i < 10; i++) {
    int32_t a0 = distrib(gen);
    int32_t a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I32Type(),
                                             {program.I32Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.SubI32(program.ConstI32(a0), args[0]);
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int32_t(int32_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int32_t res = a0 - a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST_P(BackendTest, I32_SubConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);

  for (int i = 0; i < 10; i++) {
    int32_t a0 = distrib(gen);
    int32_t a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I32Type(),
                                             {program.I32Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.SubI32(args[0], program.ConstI32(a1));
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int32_t(int32_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int32_t res = a0 - a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST_P(BackendTest, I32_MUL) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);

  ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I32Type(), {program.I32Type(), program.I32Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.MulI32(args[0], args[1]);
  program.Return(sum);

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<int32_t(int32_t, int32_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  for (int i = 0; i < 10; i++) {
    int32_t a0 = distrib(gen);
    int32_t a1 = distrib(gen);
    int32_t res = a0 * a1;
    EXPECT_EQ(res, compute(a0, a1));
  }
}

TEST_P(BackendTest, I32_MULConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);

  for (int i = 0; i < 10; i++) {
    int32_t a0 = distrib(gen);
    int32_t a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I32Type(),
                                             {program.I32Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.MulI32(program.ConstI32(a0), args[0]);
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int32_t(int32_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int32_t res = a0 * a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST_P(BackendTest, I32_MULConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);

  for (int i = 0; i < 10; i++) {
    int32_t a0 = distrib(gen);
    int32_t a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I32Type(),
                                             {program.I32Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.MulI32(args[0], program.ConstI32(a1));
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int32_t(int32_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int32_t res = a0 * a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST_P(BackendTest, I32_CONST) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    int32_t c = distrib(gen);

    ProgramBuilder program;
    program.CreatePublicFunction(program.I32Type(), {}, "compute");
    program.Return(program.ConstI32(c));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int32_t()>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(c, compute());
  }
}

TEST_P(BackendTest, I32_ZEXT_I64) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I64Type(),
                                             {program.I32Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.I64ZextI32(args[0]));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int64_t(int32_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int32_t c = distrib(gen);
    int64_t zexted = int64_t(c) & 0xFFFFFFFF;
    EXPECT_EQ(zexted, compute(c));
  }
}

TEST_P(BackendTest, I32_ZEXT_I64Const) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    int32_t c = distrib(gen);

    ProgramBuilder program;
    program.CreatePublicFunction(program.I64Type(), {program.I32Type()},
                                 "compute");
    program.Return(program.I64ZextI32(program.ConstI32(c)));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int64_t(int32_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int64_t zexted = int64_t(c) & 0xFFFFFFFF;
    EXPECT_EQ(zexted, compute(c));
  }
}

TEST_P(BackendTest, I32_CONV_F64) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.F64Type(),
                                             {program.I32Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.F64ConvI32(args[0]));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<double(int32_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int32_t c = distrib(gen);
    double conv = c;
    EXPECT_EQ(conv, compute(c));
  }
}

TEST_P(BackendTest, I32_CONV_F64Const) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    int32_t c = distrib(gen);

    ProgramBuilder program;
    program.CreatePublicFunction(program.F64Type(), {program.I32Type()},
                                 "compute");
    program.Return(program.F64ConvI32(program.ConstI32(c)));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<double(int32_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    double conv = c;
    EXPECT_EQ(conv, compute(c));
  }
}

TEST_P(BackendTest, I32_CMP_XXReturn) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type : {CompType::EQ, CompType::NE, CompType::LT,
                          CompType::LE, CompType::GT, CompType::GE}) {
      int32_t c1 = distrib(gen);
      int32_t c2 = distrib(gen);

      ProgramBuilder program;
      auto func = program.CreatePublicFunction(
          program.I1Type(), {program.I32Type(), program.I32Type()}, "compute");
      auto args = program.GetFunctionArguments(func);
      program.Return(program.CmpI32(cmp_type, args[0], args[1]));

      auto backend = Compile(GetParam(), program);

      using compute_fn = std::add_pointer<int8_t(int32_t, int32_t)>::type;
      auto compute =
          reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

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

TEST_P(BackendTest, I32_CMP_XXBranch) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type : {CompType::EQ, CompType::NE, CompType::LT,
                          CompType::LE, CompType::GT, CompType::GE}) {
      int32_t c1 = distrib(gen);
      int32_t c2 = distrib(gen);

      ProgramBuilder program;
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

      auto backend = Compile(GetParam(), program);

      using compute_fn = std::add_pointer<int8_t(int32_t, int32_t)>::type;
      auto compute =
          reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

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

TEST_P(BackendTest, I32_CMP_XXConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type : {CompType::EQ, CompType::NE, CompType::LT,
                          CompType::LE, CompType::GT, CompType::GE}) {
      int32_t c1 = distrib(gen);
      int32_t c2 = distrib(gen);

      ProgramBuilder program;
      auto func = program.CreatePublicFunction(program.I1Type(),
                                               {program.I32Type()}, "compute");
      auto args = program.GetFunctionArguments(func);
      program.Return(program.CmpI32(cmp_type, program.ConstI32(c1), args[0]));

      auto backend = Compile(GetParam(), program);

      using compute_fn = std::add_pointer<int8_t(int32_t)>::type;
      auto compute =
          reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

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

TEST_P(BackendTest, I32_CMP_XXConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type : {CompType::EQ, CompType::NE, CompType::LT,
                          CompType::LE, CompType::GT, CompType::GE}) {
      int32_t c1 = distrib(gen);
      int32_t c2 = distrib(gen);

      ProgramBuilder program;
      auto func = program.CreatePublicFunction(program.I1Type(),
                                               {program.I32Type()}, "compute");
      auto args = program.GetFunctionArguments(func);
      program.Return(program.CmpI32(cmp_type, args[0], program.ConstI32(c2)));

      auto backend = Compile(GetParam(), program);

      using compute_fn = std::add_pointer<int8_t(int32_t)>::type;
      auto compute =
          reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

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

TEST_P(BackendTest, I32_LOAD) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    int32_t loc = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(
        program.I32Type(), {program.PointerType(program.I32Type())}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.LoadI32(args[0]));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int32_t(int32_t*)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(loc, compute(&loc));
  }
}

TEST_P(BackendTest, I32_LOADGlobal) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    int32_t c = distrib(gen);

    ProgramBuilder program;
    auto global =
        program.Global(true, false, program.I32Type(), program.ConstI32(c));
    program.CreatePublicFunction(program.I32Type(), {}, "compute");
    program.Return(program.LoadI32(global));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int32_t()>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(c, compute());
  }
}

TEST_P(BackendTest, I32_LOADStruct) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    int32_t c = distrib(gen);

    struct Test {
      int32_t x;
    };

    ProgramBuilder program;
    auto st = program.StructType({program.I32Type()});
    auto func = program.CreatePublicFunction(
        program.I32Type(), {program.PointerType(st)}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.LoadI32(program.ConstGEP(st, args[0], {0, 0})));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int32_t(Test*)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    Test t{.x = c};
    EXPECT_EQ(c, compute(&t));
  }
}

TEST_P(BackendTest, I32_STORE) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    int32_t c = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(
        program.VoidType(),
        {program.PointerType(program.I32Type()), program.I32Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreI32(args[0], args[1]);
    program.Return();

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<void(int32_t*, int32_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int32_t loc;
    compute(&loc, c);
    EXPECT_EQ(loc, c);
  }
}

TEST_P(BackendTest, I32_STOREConst) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    int32_t c = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.PointerType(program.I32Type())},
        "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreI32(args[0], program.ConstI32(c));
    program.Return();

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<void(int32_t*)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int32_t loc;
    compute(&loc);
    EXPECT_EQ(loc, c);
  }
}

TEST_P(BackendTest, I32_STOREGlobal) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    int32_t c = distrib(gen);

    ProgramBuilder program;
    auto global =
        program.Global(true, false, program.I32Type(), program.ConstI32(c));
    auto func = program.CreatePublicFunction(
        program.PointerType(program.I32Type()), {program.I32Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreI32(global, args[0]);
    program.Return(global);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int32_t*(int32_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    auto loc = compute(c);
    EXPECT_EQ(*loc, c);
  }
}

TEST_P(BackendTest, I32_STOREStruct) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    int32_t c = distrib(gen);

    struct Test {
      int32_t x;
    };

    ProgramBuilder program;
    auto st = program.StructType({program.I32Type()});
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.PointerType(st), program.I32Type()},
        "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreI32(program.ConstGEP(st, args[0], {0, 0}), args[1]);
    program.Return();

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<void(Test*, int32_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    Test t;
    compute(&t, c);
    EXPECT_EQ(t.x, c);
  }
}

TEST_P(BackendTest, PHII32) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    int32_t c1 = distrib(gen);
    int32_t c2 = distrib(gen);

    ProgramBuilder program;
    auto type = program.I32Type();
    auto func = program.CreatePublicFunction(
        type, {program.I1Type(), type, type}, "compute");
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

    auto backend = Compile(GetParam(), program);

    using compute_fn =
        std::add_pointer<int32_t(int8_t, int32_t, int32_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(c1, compute(1, c1, c2));
    EXPECT_EQ(c2, compute(0, c1, c2));
  }
}

TEST_P(BackendTest, PHII32ConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    int32_t c1 = distrib(gen);
    int32_t c2 = distrib(gen);

    ProgramBuilder program;
    auto type = program.I32Type();
    auto func =
        program.CreatePublicFunction(type, {program.I1Type(), type}, "compute");
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

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int32_t(int8_t, int32_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(c1, compute(1, c2));
    EXPECT_EQ(c2, compute(0, c2));
  }
}

TEST_P(BackendTest, PHII32ConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int32_t> distrib(INT32_MIN, INT32_MAX);
  for (int i = 0; i < 10; i++) {
    int32_t c1 = distrib(gen);
    int32_t c2 = distrib(gen);

    ProgramBuilder program;
    auto type = program.I32Type();
    auto func =
        program.CreatePublicFunction(type, {program.I1Type(), type}, "compute");
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

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int32_t(int8_t, int32_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(c1, compute(1, c1));
    EXPECT_EQ(c2, compute(0, c1));
  }
}

TEST_P(BackendTest, I64_ADD) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);

  ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I64Type(), {program.I64Type(), program.I64Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.AddI64(args[0], args[1]);
  program.Return(sum);

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<int64_t(int64_t, int64_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  for (int i = 0; i < 10; i++) {
    int64_t a0 = distrib(gen);
    int64_t a1 = distrib(gen);
    int64_t res = a0 + a1;
    EXPECT_EQ(res, compute(a0, a1));
  }
}

TEST_P(BackendTest, I64_ADDConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);

  for (int i = 0; i < 10; i++) {
    int64_t a0 = distrib(gen);
    int64_t a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I64Type(),
                                             {program.I64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.AddI64(program.ConstI64(a0), args[0]);
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int64_t(int64_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int64_t res = a0 + a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST_P(BackendTest, I64_ADDConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);

  for (int i = 0; i < 10; i++) {
    int64_t a0 = distrib(gen);
    int64_t a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I64Type(),
                                             {program.I64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.AddI64(args[0], program.ConstI64(a1));
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int64_t(int64_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int64_t res = a0 + a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST_P(BackendTest, I64_AND) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);

  ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I64Type(), {program.I64Type(), program.I64Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.AndI64(args[0], args[1]);
  program.Return(sum);

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<int64_t(int64_t, int64_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  for (int i = 0; i < 10; i++) {
    int64_t a0 = distrib(gen);
    int64_t a1 = distrib(gen);
    int64_t res = a0 & a1;
    EXPECT_EQ(res, compute(a0, a1));
  }
}

TEST_P(BackendTest, I64_ANDConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);

  for (int i = 0; i < 10; i++) {
    int64_t a0 = distrib(gen);
    int64_t a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I64Type(),
                                             {program.I64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.AndI64(program.ConstI64(a0), args[0]);
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int64_t(int64_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int64_t res = a0 & a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST_P(BackendTest, I64_ANDConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);

  for (int i = 0; i < 10; i++) {
    int64_t a0 = distrib(gen);
    int64_t a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I64Type(),
                                             {program.I64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.AndI64(args[0], program.ConstI64(a1));
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int64_t(int64_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int64_t res = a0 & a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST_P(BackendTest, I64_OR) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);

  ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I64Type(), {program.I64Type(), program.I64Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.OrI64(args[0], args[1]);
  program.Return(sum);

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<int64_t(int64_t, int64_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  for (int i = 0; i < 10; i++) {
    int64_t a0 = distrib(gen);
    int64_t a1 = distrib(gen);
    int64_t res = a0 | a1;
    EXPECT_EQ(res, compute(a0, a1));
  }
}

TEST_P(BackendTest, I64_ORConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);

  for (int i = 0; i < 10; i++) {
    int64_t a0 = distrib(gen);
    int64_t a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I64Type(),
                                             {program.I64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.OrI64(program.ConstI64(a0), args[0]);
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int64_t(int64_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int64_t res = a0 | a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST_P(BackendTest, I64_ORConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);

  for (int i = 0; i < 10; i++) {
    int64_t a0 = distrib(gen);
    int64_t a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I64Type(),
                                             {program.I64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.OrI64(args[0], program.ConstI64(a1));
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int64_t(int64_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int64_t res = a0 | a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST_P(BackendTest, I64_LSHIFT) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  std::uniform_int_distribution<uint8_t> shift_distrib(0, 63);

  for (int i = 0; i < 10; i++) {
    int64_t a0 = distrib(gen);
    uint8_t a1 = shift_distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I64Type(),
                                             {program.I64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.LShiftI64(args[0], a1);
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int64_t(int64_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int64_t res = a0 << a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST_P(BackendTest, I64_LSHIFTConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  std::uniform_int_distribution<uint8_t> shift_distrib(0, 63);

  for (int i = 0; i < 10; i++) {
    int64_t a0 = distrib(gen);
    uint8_t a1 = shift_distrib(gen);

    ProgramBuilder program;
    program.CreatePublicFunction(program.I64Type(), {}, "compute");
    auto sum = program.LShiftI64(program.ConstI64(a0), a1);
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int64_t()>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int64_t res = a0 << a1;
    EXPECT_EQ(res, compute());
  }
}

TEST_P(BackendTest, I64_RSHIFT) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  std::uniform_int_distribution<uint8_t> shift_distrib(0, 63);

  for (int i = 0; i < 10; i++) {
    uint64_t a0 = distrib(gen);
    uint8_t a1 = shift_distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I64Type(),
                                             {program.I64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.RShiftI64(args[0], a1);
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int64_t(int64_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int64_t res = a0 >> a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST_P(BackendTest, I64_RSHIFTConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  std::uniform_int_distribution<uint8_t> shift_distrib(0, 63);

  for (int i = 0; i < 10; i++) {
    uint64_t a0 = distrib(gen);
    uint8_t a1 = shift_distrib(gen);

    ProgramBuilder program;
    program.CreatePublicFunction(program.I64Type(), {}, "compute");
    auto sum = program.RShiftI64(program.ConstI64(a0), a1);
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int64_t()>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int64_t res = a0 >> a1;
    EXPECT_EQ(res, compute());
  }
}

TEST_P(BackendTest, I64_SUB) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);

  ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I64Type(), {program.I64Type(), program.I64Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.SubI64(args[0], args[1]);
  program.Return(sum);

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<int64_t(int64_t, int64_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  for (int i = 0; i < 10; i++) {
    int64_t a0 = distrib(gen);
    int64_t a1 = distrib(gen);
    int64_t res = a0 - a1;
    EXPECT_EQ(res, compute(a0, a1));
  }
}

TEST_P(BackendTest, I64_SubConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);

  for (int i = 0; i < 10; i++) {
    int64_t a0 = distrib(gen);
    int64_t a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I64Type(),
                                             {program.I64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.SubI64(program.ConstI64(a0), args[0]);
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int64_t(int64_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int64_t res = a0 - a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST_P(BackendTest, I64_SubConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);

  for (int i = 0; i < 10; i++) {
    int64_t a0 = distrib(gen);
    int64_t a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I64Type(),
                                             {program.I64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.SubI64(args[0], program.ConstI64(a1));
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int64_t(int64_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int64_t res = a0 - a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST_P(BackendTest, I64_MUL) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);

  ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I64Type(), {program.I64Type(), program.I64Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.MulI64(args[0], args[1]);
  program.Return(sum);

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<int64_t(int64_t, int64_t)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  for (int i = 0; i < 10; i++) {
    int64_t a0 = distrib(gen);
    int64_t a1 = distrib(gen);
    int64_t res = a0 * a1;
    EXPECT_EQ(res, compute(a0, a1));
  }
}

TEST_P(BackendTest, I64_MULConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);

  for (int i = 0; i < 10; i++) {
    int64_t a0 = distrib(gen);
    int64_t a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I64Type(),
                                             {program.I64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.MulI64(program.ConstI64(a0), args[0]);
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int64_t(int64_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int64_t res = a0 * a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST_P(BackendTest, I64_MULConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);

  for (int i = 0; i < 10; i++) {
    int64_t a0 = distrib(gen);
    int64_t a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I64Type(),
                                             {program.I64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.MulI64(args[0], program.ConstI64(a1));
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int64_t(int64_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int64_t res = a0 * a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST_P(BackendTest, I64_CONST) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    int64_t c = distrib(gen);

    ProgramBuilder program;
    program.CreatePublicFunction(program.I64Type(), {}, "compute");
    program.Return(program.ConstI64(c));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int64_t()>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(c, compute());
  }
}

TEST_P(BackendTest, I64_CONV_F64) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.F64Type(),
                                             {program.I64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.F64ConvI64(args[0]));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<double(int64_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int64_t c = distrib(gen);
    double conv = c;
    EXPECT_EQ(conv, compute(c));
  }
}

TEST_P(BackendTest, I64_CONV_F64Const) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    int64_t c = distrib(gen);

    ProgramBuilder program;
    program.CreatePublicFunction(program.F64Type(), {program.I64Type()},
                                 "compute");
    program.Return(program.F64ConvI64(program.ConstI64(c)));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<double()>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    double conv = c;
    EXPECT_EQ(conv, compute());
  }
}

TEST_P(BackendTest, I64_CMP_XXReturn) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type : {CompType::EQ, CompType::NE, CompType::LT,
                          CompType::LE, CompType::GT, CompType::GE}) {
      int64_t c1 = distrib(gen);
      int64_t c2 = distrib(gen);

      ProgramBuilder program;
      auto func = program.CreatePublicFunction(
          program.I1Type(), {program.I64Type(), program.I64Type()}, "compute");
      auto args = program.GetFunctionArguments(func);
      program.Return(program.CmpI64(cmp_type, args[0], args[1]));

      auto backend = Compile(GetParam(), program);

      using compute_fn = std::add_pointer<int8_t(int64_t, int64_t)>::type;
      auto compute =
          reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

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

TEST_P(BackendTest, PTR_CMP_NULLPTRReturn) {
  ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.I1Type(), {program.PointerType(program.I64Type())}, "compute");
  auto args = program.GetFunctionArguments(func);
  program.Return(program.IsNullPtr(args[0]));

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<int8_t(int64_t*)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  EXPECT_NE(0, compute(nullptr));
  int64_t value = 0xDEADBEEF;
  int64_t* ptr = reinterpret_cast<int64_t*>(value);
  EXPECT_EQ(0, compute(ptr));
}

TEST_P(BackendTest, I64_CMP_XXBranch) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type : {CompType::EQ, CompType::NE, CompType::LT,
                          CompType::LE, CompType::GT, CompType::GE}) {
      int64_t c1 = distrib(gen);
      int64_t c2 = distrib(gen);

      ProgramBuilder program;
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

      auto backend = Compile(GetParam(), program);

      using compute_fn = std::add_pointer<int8_t(int64_t, int64_t)>::type;
      auto compute =
          reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

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

TEST_P(BackendTest, I64_CMP_XXConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type : {CompType::EQ, CompType::NE, CompType::LT,
                          CompType::LE, CompType::GT, CompType::GE}) {
      int64_t c1 = distrib(gen);
      int64_t c2 = distrib(gen);

      ProgramBuilder program;
      auto func = program.CreatePublicFunction(program.I1Type(),
                                               {program.I64Type()}, "compute");
      auto args = program.GetFunctionArguments(func);
      program.Return(program.CmpI64(cmp_type, program.ConstI64(c1), args[0]));

      auto backend = Compile(GetParam(), program);

      using compute_fn = std::add_pointer<int8_t(int64_t)>::type;
      auto compute =
          reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

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

TEST_P(BackendTest, I64_CMP_XXConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type : {CompType::EQ, CompType::NE, CompType::LT,
                          CompType::LE, CompType::GT, CompType::GE}) {
      int64_t c1 = distrib(gen);
      int64_t c2 = distrib(gen);

      ProgramBuilder program;
      auto func = program.CreatePublicFunction(program.I1Type(),
                                               {program.I64Type()}, "compute");
      auto args = program.GetFunctionArguments(func);
      program.Return(program.CmpI64(cmp_type, args[0], program.ConstI64(c2)));

      auto backend = Compile(GetParam(), program);

      using compute_fn = std::add_pointer<int8_t(int64_t)>::type;
      auto compute =
          reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

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

TEST_P(BackendTest, I64_LOAD) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    int64_t loc = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(
        program.I64Type(), {program.PointerType(program.I64Type())}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.LoadI64(args[0]));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int64_t(int64_t*)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(loc, compute(&loc));
  }
}

TEST_P(BackendTest, I64_LOADGlobal) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    int64_t c = distrib(gen);

    ProgramBuilder program;
    auto global =
        program.Global(true, false, program.I64Type(), program.ConstI64(c));
    program.CreatePublicFunction(program.I64Type(), {}, "compute");
    program.Return(program.LoadI64(global));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int64_t()>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(c, compute());
  }
}

TEST_P(BackendTest, I64_LOADStruct) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    int64_t c = distrib(gen);

    struct Test {
      int64_t x;
    };

    ProgramBuilder program;
    auto st = program.StructType({program.I64Type()});
    auto func = program.CreatePublicFunction(
        program.I64Type(), {program.PointerType(st)}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.LoadI64(program.ConstGEP(st, args[0], {0, 0})));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int64_t(Test*)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    Test t{.x = c};
    EXPECT_EQ(c, compute(&t));
  }
}

TEST_P(BackendTest, I64_STORE) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    int64_t c = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(
        program.VoidType(),
        {program.PointerType(program.I64Type()), program.I64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreI64(args[0], args[1]);
    program.Return();

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<void(int64_t*, int64_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int64_t loc;
    compute(&loc, c);
    EXPECT_EQ(loc, c);
  }
}

TEST_P(BackendTest, I64_STOREConst) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    int64_t c = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.PointerType(program.I64Type())},
        "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreI64(args[0], program.ConstI64(c));
    program.Return();

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<void(int64_t*)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int64_t loc;
    compute(&loc);
    EXPECT_EQ(loc, c);
  }
}

TEST_P(BackendTest, I64_STOREGlobal) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    int64_t c = distrib(gen);

    ProgramBuilder program;
    auto global =
        program.Global(true, false, program.I64Type(), program.ConstI64(c));
    auto func = program.CreatePublicFunction(
        program.PointerType(program.I64Type()), {program.I64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreI64(global, args[0]);
    program.Return(global);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int64_t*(int64_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    auto loc = compute(c);
    EXPECT_EQ(*loc, c);
  }
}

TEST_P(BackendTest, I64_STOREStruct) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    int64_t c = distrib(gen);

    struct Test {
      int64_t x;
    };

    ProgramBuilder program;
    auto st = program.StructType({program.I64Type()});
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.PointerType(st), program.I64Type()},
        "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreI64(program.ConstGEP(st, args[0], {0, 0}), args[1]);
    program.Return();

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<void(Test*, int64_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    Test t;
    compute(&t, c);
    EXPECT_EQ(t.x, c);
  }
}

TEST_P(BackendTest, PHII64) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    int64_t c1 = distrib(gen);
    int64_t c2 = distrib(gen);

    ProgramBuilder program;
    auto type = program.I64Type();
    auto func = program.CreatePublicFunction(
        type, {program.I1Type(), type, type}, "compute");
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

    auto backend = Compile(GetParam(), program);

    using compute_fn =
        std::add_pointer<int64_t(int8_t, int64_t, int64_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(c1, compute(1, c1, c2));
    EXPECT_EQ(c2, compute(0, c1, c2));
  }
}

TEST_P(BackendTest, PHII64ConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    int64_t c1 = distrib(gen);
    int64_t c2 = distrib(gen);

    ProgramBuilder program;
    auto type = program.I64Type();
    auto func =
        program.CreatePublicFunction(type, {program.I1Type(), type}, "compute");
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

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int64_t(int8_t, int64_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(c1, compute(1, c2));
    EXPECT_EQ(c2, compute(0, c2));
  }
}

TEST_P(BackendTest, PHII64ConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> distrib(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 10; i++) {
    int64_t c1 = distrib(gen);
    int64_t c2 = distrib(gen);

    ProgramBuilder program;
    auto type = program.I64Type();
    auto func =
        program.CreatePublicFunction(type, {program.I1Type(), type}, "compute");
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

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int64_t(int8_t, int64_t)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(c1, compute(1, c1));
    EXPECT_EQ(c2, compute(0, c1));
  }
}

TEST_P(BackendTest, F64_ADD) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);

  ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.F64Type(), {program.F64Type(), program.F64Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.AddF64(args[0], args[1]);
  program.Return(sum);

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<double(double, double)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  for (int i = 0; i < 10; i++) {
    double a0 = distrib(gen);
    double a1 = distrib(gen);
    double res = a0 + a1;
    EXPECT_EQ(res, compute(a0, a1));
  }
}

TEST_P(BackendTest, F64_ADDConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);

  for (int i = 0; i < 10; i++) {
    double a0 = distrib(gen);
    double a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.F64Type(),
                                             {program.F64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.AddF64(program.ConstF64(a0), args[0]);
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<double(double)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    double res = a0 + a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST_P(BackendTest, F64_ADDConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);

  for (int i = 0; i < 10; i++) {
    double a0 = distrib(gen);
    double a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.F64Type(),
                                             {program.F64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.AddF64(args[0], program.ConstF64(a1));
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<double(double)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    double res = a0 + a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST_P(BackendTest, F64_SUB) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);

  ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.F64Type(), {program.F64Type(), program.F64Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.SubF64(args[0], args[1]);
  program.Return(sum);

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<double(double, double)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  for (int i = 0; i < 10; i++) {
    double a0 = distrib(gen);
    double a1 = distrib(gen);
    double res = a0 - a1;
    EXPECT_EQ(res, compute(a0, a1));
  }
}

TEST_P(BackendTest, F64_SubConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);

  for (int i = 0; i < 10; i++) {
    double a0 = distrib(gen);
    double a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.F64Type(),
                                             {program.F64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.SubF64(program.ConstF64(a0), args[0]);
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<double(double)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    double res = a0 - a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST_P(BackendTest, F64_SubConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);

  for (int i = 0; i < 10; i++) {
    double a0 = distrib(gen);
    double a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.F64Type(),
                                             {program.F64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.SubF64(args[0], program.ConstF64(a1));
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<double(double)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    double res = a0 - a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST_P(BackendTest, F64_MUL) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);

  ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.F64Type(), {program.F64Type(), program.F64Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.MulF64(args[0], args[1]);
  program.Return(sum);

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<double(double, double)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  for (int i = 0; i < 10; i++) {
    double a0 = distrib(gen);
    double a1 = distrib(gen);
    double res = a0 * a1;
    EXPECT_EQ(res, compute(a0, a1));
  }
}

TEST_P(BackendTest, F64_MULConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);

  for (int i = 0; i < 10; i++) {
    double a0 = distrib(gen);
    double a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.F64Type(),
                                             {program.F64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.MulF64(program.ConstF64(a0), args[0]);
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<double(double)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    double res = a0 * a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST_P(BackendTest, F64_MULConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);

  for (int i = 0; i < 10; i++) {
    double a0 = distrib(gen);
    double a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.F64Type(),
                                             {program.F64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.MulF64(args[0], program.ConstF64(a1));
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<double(double)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    double res = a0 * a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST_P(BackendTest, F64_DIV) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);

  ProgramBuilder program;
  auto func = program.CreatePublicFunction(
      program.F64Type(), {program.F64Type(), program.F64Type()}, "compute");
  auto args = program.GetFunctionArguments(func);
  auto sum = program.DivF64(args[0], args[1]);
  program.Return(sum);

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<double(double, double)>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  for (int i = 0; i < 10; i++) {
    double a0 = distrib(gen);
    double a1 = distrib(gen);
    double res = a0 / a1;
    EXPECT_EQ(res, compute(a0, a1));
  }
}

TEST_P(BackendTest, F64_DIVConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);

  for (int i = 0; i < 10; i++) {
    double a0 = distrib(gen);
    double a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.F64Type(),
                                             {program.F64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.DivF64(program.ConstF64(a0), args[0]);
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<double(double)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    double res = a0 / a1;
    EXPECT_EQ(res, compute(a1));
  }
}

TEST_P(BackendTest, F64_DIVConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);

  for (int i = 0; i < 10; i++) {
    double a0 = distrib(gen);
    double a1 = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.F64Type(),
                                             {program.F64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    auto sum = program.DivF64(args[0], program.ConstF64(a1));
    program.Return(sum);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<double(double)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    double res = a0 / a1;
    EXPECT_EQ(res, compute(a0));
  }
}

TEST_P(BackendTest, F64_CONST) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    double c = distrib(gen);

    ProgramBuilder program;
    program.CreatePublicFunction(program.F64Type(), {}, "compute");
    program.Return(program.ConstF64(c));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<double()>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(c, compute());
  }
}

TEST_P(BackendTest, F64_CONV_I64) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-9223372036854775808.0,
                                                 9223372036854775807.0);
  for (int i = 0; i < 10; i++) {
    ProgramBuilder program;
    auto func = program.CreatePublicFunction(program.I64Type(),
                                             {program.F64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.I64ConvF64(args[0]));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int64_t(double)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    double c = distrib(gen);
    int64_t conv = c;
    EXPECT_EQ(conv, compute(c));
  }
}

TEST_P(BackendTest, F64_CONV_I64Const) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-9223372036854775808.0,
                                                 9223372036854775807.0);
  for (int i = 0; i < 10; i++) {
    double c = distrib(gen);

    ProgramBuilder program;
    program.CreatePublicFunction(program.I64Type(), {}, "compute");
    program.Return(program.I64ConvF64(program.ConstF64(c)));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<int64_t()>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    int64_t conv = c;
    EXPECT_EQ(conv, compute());
  }
}

TEST_P(BackendTest, F64_CMP_XXReturn) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type : {CompType::EQ, CompType::NE, CompType::LT,
                          CompType::LE, CompType::GT, CompType::GE}) {
      double c1 = distrib(gen);
      double c2 = distrib(gen);

      ProgramBuilder program;
      auto func = program.CreatePublicFunction(
          program.I1Type(), {program.F64Type(), program.F64Type()}, "compute");
      auto args = program.GetFunctionArguments(func);
      program.Return(program.CmpF64(cmp_type, args[0], args[1]));

      auto backend = Compile(GetParam(), program);

      using compute_fn = std::add_pointer<int8_t(double, double)>::type;
      auto compute =
          reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

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

TEST_P(BackendTest, F64_CMP_XXBranch) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type : {CompType::EQ, CompType::NE, CompType::LT,
                          CompType::LE, CompType::GT, CompType::GE}) {
      double c1 = distrib(gen);
      double c2 = distrib(gen);

      ProgramBuilder program;
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

      auto backend = Compile(GetParam(), program);

      using compute_fn = std::add_pointer<int8_t(double, double)>::type;
      auto compute =
          reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

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

TEST_P(BackendTest, F64_CMP_XXConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type : {CompType::EQ, CompType::NE, CompType::LT,
                          CompType::LE, CompType::GT, CompType::GE}) {
      double c1 = distrib(gen);
      double c2 = distrib(gen);

      ProgramBuilder program;
      auto func = program.CreatePublicFunction(program.I1Type(),
                                               {program.F64Type()}, "compute");
      auto args = program.GetFunctionArguments(func);
      program.Return(program.CmpF64(cmp_type, program.ConstF64(c1), args[0]));

      auto backend = Compile(GetParam(), program);

      using compute_fn = std::add_pointer<int8_t(double)>::type;
      auto compute =
          reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

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

TEST_P(BackendTest, F64_CMP_XXConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    for (auto cmp_type : {CompType::EQ, CompType::NE, CompType::LT,
                          CompType::LE, CompType::GT, CompType::GE}) {
      double c1 = distrib(gen);
      double c2 = distrib(gen);

      ProgramBuilder program;
      auto func = program.CreatePublicFunction(program.I1Type(),
                                               {program.F64Type()}, "compute");
      auto args = program.GetFunctionArguments(func);
      program.Return(program.CmpF64(cmp_type, args[0], program.ConstF64(c2)));

      auto backend = Compile(GetParam(), program);

      using compute_fn = std::add_pointer<int8_t(double)>::type;
      auto compute =
          reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

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

TEST_P(BackendTest, F64_LOAD) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    double loc = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(
        program.F64Type(), {program.PointerType(program.F64Type())}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.LoadF64(args[0]));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<double(double*)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(loc, compute(&loc));
  }
}

TEST_P(BackendTest, F64_LOADGlobal) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    double c = distrib(gen);

    ProgramBuilder program;
    auto global =
        program.Global(true, false, program.F64Type(), program.ConstF64(c));
    program.CreatePublicFunction(program.F64Type(), {}, "compute");
    program.Return(
        program.LoadF64(program.ConstGEP(program.F64Type(), global, {0})));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<double()>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(c, compute());
  }
}

TEST_P(BackendTest, F64_LOADStruct) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    double c = distrib(gen);

    struct Test {
      double x;
    };

    ProgramBuilder program;
    auto st = program.StructType({program.F64Type()});
    auto func = program.CreatePublicFunction(
        program.F64Type(), {program.PointerType(st)}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.Return(program.LoadF64(program.ConstGEP(st, args[0], {0, 0})));

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<double(Test*)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    Test t{.x = c};
    EXPECT_EQ(c, compute(&t));
  }
}

TEST_P(BackendTest, F64_STORE) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    double c = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(
        program.VoidType(),
        {program.PointerType(program.F64Type()), program.F64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreF64(args[0], args[1]);
    program.Return();

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<void(double*, double)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    double loc;
    compute(&loc, c);
    EXPECT_EQ(loc, c);
  }
}

TEST_P(BackendTest, F64_STOREConst) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    double c = distrib(gen);

    ProgramBuilder program;
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.PointerType(program.F64Type())},
        "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreF64(args[0], program.ConstF64(c));
    program.Return();

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<void(double*)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    double loc;
    compute(&loc);
    EXPECT_EQ(loc, c);
  }
}

TEST_P(BackendTest, F64_STOREGlobal) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    double c = distrib(gen);

    ProgramBuilder program;
    auto global =
        program.Global(true, false, program.F64Type(), program.ConstF64(c));
    auto func = program.CreatePublicFunction(
        program.PointerType(program.F64Type()), {program.F64Type()}, "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreF64(program.ConstGEP(program.F64Type(), global, {0}), args[0]);
    program.Return(global);

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<double*(double)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    auto loc = compute(c);
    EXPECT_EQ(*loc, c);
  }
}

TEST_P(BackendTest, F64_STOREStruct) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    double c = distrib(gen);

    struct Test {
      double x;
    };

    ProgramBuilder program;
    auto st = program.StructType({program.F64Type()});
    auto func = program.CreatePublicFunction(
        program.VoidType(), {program.PointerType(st), program.F64Type()},
        "compute");
    auto args = program.GetFunctionArguments(func);
    program.StoreF64(program.ConstGEP(st, args[0], {0, 0}), args[1]);
    program.Return();

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<void(Test*, double)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    Test t;
    compute(&t, c);
    EXPECT_EQ(t.x, c);
  }
}

TEST_P(BackendTest, PHIF64) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    double c1 = distrib(gen);
    double c2 = distrib(gen);

    ProgramBuilder program;
    auto type = program.F64Type();
    auto func = program.CreatePublicFunction(
        type, {program.I1Type(), type, type}, "compute");
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

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<double(int8_t, double, double)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(c1, compute(1, c1, c2));
    EXPECT_EQ(c2, compute(0, c1, c2));
  }
}

TEST_P(BackendTest, PHIF64ConstArg0) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    double c1 = distrib(gen);
    double c2 = distrib(gen);

    ProgramBuilder program;
    auto type = program.F64Type();
    auto func =
        program.CreatePublicFunction(type, {program.I1Type(), type}, "compute");
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

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<double(int8_t, double)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(c1, compute(1, c2));
    EXPECT_EQ(c2, compute(0, c2));
  }
}

TEST_P(BackendTest, PHIF64ConstArg1) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> distrib(-1e100, 1e100);
  for (int i = 0; i < 10; i++) {
    double c1 = distrib(gen);
    double c2 = distrib(gen);

    ProgramBuilder program;
    auto type = program.F64Type();
    auto func =
        program.CreatePublicFunction(type, {program.I1Type(), type}, "compute");
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

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<double(int8_t, double)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    EXPECT_EQ(c1, compute(1, c1));
    EXPECT_EQ(c2, compute(0, c1));
  }
}

TEST_P(BackendTest, Loop) {
  ProgramBuilder program;
  program.CreatePublicFunction(program.I32Type(), {}, "compute");

  auto bb1 = program.GenerateBlock();
  auto bb2 = program.GenerateBlock();
  auto bb3 = program.GenerateBlock();

  auto init = program.ConstI32(0);
  auto suminit = program.ConstI32(0);
  auto phi_1 = program.PhiMember(init);
  auto sumphi_1 = program.PhiMember(suminit);
  program.Branch(bb1);

  program.SetCurrentBlock(bb1);
  auto phi = program.Phi(program.I32Type());
  auto sumphi = program.Phi(program.I32Type());
  program.UpdatePhiMember(phi, phi_1);
  program.UpdatePhiMember(sumphi, sumphi_1);
  auto cond = program.CmpI32(CompType::LT, phi, program.ConstI32(10));
  program.Branch(cond, bb2, bb3);

  program.SetCurrentBlock(bb2);
  auto incr = program.AddI32(phi, program.ConstI32(1));
  auto newsum = program.AddI32(sumphi, phi);
  auto phi_2 = program.PhiMember(incr);
  program.UpdatePhiMember(phi, phi_2);
  auto sumphi_2 = program.PhiMember(newsum);
  program.UpdatePhiMember(sumphi, sumphi_2);
  program.Branch(bb1);

  program.SetCurrentBlock(bb3);
  auto scaled = program.MulI32(sumphi, program.ConstI32(3));
  program.Return(scaled);

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<int32_t()>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  int32_t result = 0;
  for (int i = 0; i < 10; i++) {
    result += i;
  }
  result *= 3;
  EXPECT_EQ(result, compute());
}

TEST_P(BackendTest, LoopVariableAccess) {
  ProgramBuilder program;
  program.CreatePublicFunction(program.I32Type(), {}, "compute");

  auto bb1 = program.GenerateBlock();
  auto bb2 = program.GenerateBlock();
  auto bb3 = program.GenerateBlock();

  auto init = program.ConstI32(0);
  auto phi_1 = program.PhiMember(init);
  program.Branch(bb1);

  program.SetCurrentBlock(bb1);
  auto phi = program.Phi(program.I32Type());
  program.UpdatePhiMember(phi, phi_1);
  auto cond = program.CmpI32(CompType::LT, phi, program.ConstI32(10));
  program.Branch(cond, bb2, bb3);

  program.SetCurrentBlock(bb2);
  auto incr = program.AddI32(phi, program.ConstI32(1));
  auto phi_2 = program.PhiMember(incr);
  program.UpdatePhiMember(phi, phi_2);
  program.Branch(bb1);

  program.SetCurrentBlock(bb3);
  program.Return(phi);

  auto backend = Compile(GetParam(), program);

  using compute_fn = std::add_pointer<int32_t()>::type;
  auto compute = reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

  EXPECT_EQ(10, compute());
}

INSTANTIATE_TEST_SUITE_P(LLVMBackendTest, BackendTest,
                         testing::Values(std::make_pair(
                             BackendType::LLVM, RegAllocImpl::STACK_SPILL)));

INSTANTIATE_TEST_SUITE_P(ASMBackendTest_StackSpill, BackendTest,
                         testing::Values(std::make_pair(
                             BackendType::ASM, RegAllocImpl::STACK_SPILL)));

INSTANTIATE_TEST_SUITE_P(ASMBackendTest_LinearScan, BackendTest,
                         testing::Values(std::make_pair(
                             BackendType::ASM, RegAllocImpl::LINEAR_SCAN)));