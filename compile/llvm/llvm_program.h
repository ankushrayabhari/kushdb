#pragma once

#include <memory>

#include "compile/program.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

namespace kush::compile {

class LLVMImplTraits {
 public:
  using BasicBlock = llvm::BasicBlock;
  using Value = llvm::Value;
  using CompType = llvm::CmpInst::Predicate;
};

class LLVMProgram : public Program, ProgramBuilder<LLVMImplTraits> {
 public:
  LLVMProgram();
  ~LLVMProgram() = default;

  // Compile
  void Compile() const override;
  void Execute() const override;

  // Control Flow
  BasicBlock* GenerateBlock() override;
  void SetCurrentBlock(BasicBlock* b) override;
  void Branch(BasicBlock* b) override;
  void Branch(BasicBlock* b, Value* cond) override;

  // I32
  Value* AddI32(Value* v1, Value* v2) override;
  Value* MulI32(Value* v1, Value* v2) override;
  Value* DivI32(Value* v1, Value* v2) override;
  Value* SubI32(Value* v1, Value* v2) override;
  Value* Constant(int32_t v) override;

  // I64
  Value* AddI64(Value* v1, Value* v2) override;
  Value* MulI64(Value* v1, Value* v2) override;
  Value* DivI64(Value* v1, Value* v2) override;
  Value* SubI64(Value* v1, Value* v2) override;
  Value* Constant(int64_t v) override;

  // F
  Value* AddF(Value* v1, Value* v2) override;
  Value* MulF(Value* v1, Value* v2) override;
  Value* DivF(Value* v1, Value* v2) override;
  Value* SubF(Value* v1, Value* v2) override;
  Value* Constant(double v) override;

  // Comparison
  Value* CmpI(CompType cmp, Value* v1, Value* v2) override;
  Value* CmpF(CompType cmp, Value* v1, Value* v2) override;

 private:
  std::unique_ptr<llvm::LLVMContext> context_;
  std::unique_ptr<llvm::Module> module_;
  std::unique_ptr<llvm::IRBuilder<>> builder_;
};

}  // namespace kush::compile