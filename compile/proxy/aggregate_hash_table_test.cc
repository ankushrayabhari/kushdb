
#include "compile/proxy/aggregate_hash_table.h"

#include <random>

#include "gtest/gtest.h"

#include "khir/asm/asm_backend.h"
#include "khir/backend.h"
#include "khir/llvm/llvm_backend.h"
#include "khir/program_builder.h"
#include "runtime/aggregate_hash_table.h"

using namespace kush;
using namespace kush::khir;
using namespace kush::compile::proxy;

std::unique_ptr<Backend> Compile(
    const std::pair<BackendType, khir::RegAllocImpl>& params,
    khir::ProgramBuilder& program_builder) {
  auto program = program_builder.Build();
  std::unique_ptr<Backend> backend;
  switch (params.first) {
    case BackendType::ASM: {
      auto b = std::make_unique<khir::ASMBackend>(program, params.second);
      b->Compile();
      backend = std::move(b);
      break;
    }

    case BackendType::LLVM: {
      auto b = std::make_unique<khir::LLVMBackend>(program);
      b->Compile();
      backend = std::move(b);
      break;
    }
  }
  return backend;
}

class AggregateHashTableTest : public testing::TestWithParam<
                                   std::pair<BackendType, khir::RegAllocImpl>> {
};

TEST_P(AggregateHashTableTest, GetBlockIdx) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint16_t> w_distrib(0, UINT16_MAX);
  std::uniform_int_distribution<uint32_t> dw_distrib(0, UINT32_MAX);
  for (int i = 0; i < 10; i++) {
    auto salt = w_distrib(gen);
    auto offset = w_distrib(gen);
    auto idx = dw_distrib(gen);

    auto entry = runtime::AggregateHashTable::ConstructEntry(salt, offset, idx);

    ProgramBuilder program;
    auto func = program.CreateNamedFunction(
        program.I32Type(), {program.PointerType(program.I64Type())}, "compute");
    auto args = program.GetFunctionArguments(func);
    AggregateHashTableEntry ht_entry(program, args[0]);
    auto parts = ht_entry.Get();
    program.Return(parts.block_idx.Get());

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<uint32_t(uint64_t*)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));
    EXPECT_EQ(idx, compute(&entry));
  }
}

TEST_P(AggregateHashTableTest, GetSalt) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint16_t> w_distrib(0, UINT16_MAX);
  std::uniform_int_distribution<uint32_t> dw_distrib(0, UINT32_MAX);
  for (int i = 0; i < 10; i++) {
    auto salt = w_distrib(gen);
    auto offset = w_distrib(gen);
    auto idx = dw_distrib(gen);

    auto entry = runtime::AggregateHashTable::ConstructEntry(salt, offset, idx);

    ProgramBuilder program;
    auto func = program.CreateNamedFunction(
        program.I16Type(), {program.PointerType(program.I64Type())}, "compute");
    auto args = program.GetFunctionArguments(func);
    AggregateHashTableEntry ht_entry(program, args[0]);
    auto parts = ht_entry.Get();
    program.Return(parts.salt.Get());

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<uint16_t(uint64_t*)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));
    EXPECT_EQ(salt, compute(&entry));
  }
}

TEST_P(AggregateHashTableTest, GetOffset) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint16_t> w_distrib(0, UINT16_MAX);
  std::uniform_int_distribution<uint32_t> dw_distrib(0, UINT32_MAX);
  for (int i = 0; i < 10; i++) {
    auto salt = w_distrib(gen);
    auto offset = w_distrib(gen);
    auto idx = dw_distrib(gen);

    auto entry = runtime::AggregateHashTable::ConstructEntry(salt, offset, idx);

    ProgramBuilder program;
    auto func = program.CreateNamedFunction(
        program.I16Type(), {program.PointerType(program.I64Type())}, "compute");
    auto args = program.GetFunctionArguments(func);
    AggregateHashTableEntry ht_entry(program, args[0]);
    auto parts = ht_entry.Get();
    program.Return(parts.block_offset.Get());

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<uint16_t(uint64_t*)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));
    EXPECT_EQ(offset, compute(&entry));
  }
}

TEST_P(AggregateHashTableTest, Set) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint16_t> w_distrib(0, UINT16_MAX);
  std::uniform_int_distribution<uint32_t> dw_distrib(0, UINT32_MAX);
  for (int i = 0; i < 10; i++) {
    auto salt = w_distrib(gen);
    auto offset = w_distrib(gen);
    auto idx = dw_distrib(gen);

    ProgramBuilder program;
    auto func = program.CreateNamedFunction(
        program.VoidType(), {program.PointerType(program.I64Type())},
        "compute");
    auto args = program.GetFunctionArguments(func);
    AggregateHashTableEntry ht_entry(program, args[0]);
    ht_entry.Set(Int16(program, salt), Int16(program, offset),
                 Int32(program, idx));
    program.Return();

    auto backend = Compile(GetParam(), program);

    using compute_fn = std::add_pointer<void(uint64_t*)>::type;
    auto compute =
        reinterpret_cast<compute_fn>(backend->GetFunction("compute"));

    auto entry = runtime::AggregateHashTable::ConstructEntry(salt, offset, idx);
    uint64_t output;
    compute(&output);
    EXPECT_EQ(entry, output);
  }
}

INSTANTIATE_TEST_SUITE_P(LLVMBackendTest, AggregateHashTableTest,
                         testing::Values(std::make_pair(
                             BackendType::LLVM, RegAllocImpl::STACK_SPILL)));

INSTANTIATE_TEST_SUITE_P(ASMBackendTest_StackSpill, AggregateHashTableTest,
                         testing::Values(std::make_pair(
                             BackendType::ASM, RegAllocImpl::STACK_SPILL)));

INSTANTIATE_TEST_SUITE_P(ASMBackendTest_LinearScan, AggregateHashTableTest,
                         testing::Values(std::make_pair(
                             BackendType::ASM, RegAllocImpl::LINEAR_SCAN)));