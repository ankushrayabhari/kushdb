#pragma once

#include <memory>

#include "compile/program.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

namespace kush::compile {

class LLVMProgram : public Program {
 public:
  LLVMProgram();

  // Execution methods
  void Compile() const override;
  void Execute() const override;

  // Builder Methods
  llvm::BasicBlock* GenerateBlock();
  void SetCurrentBlock(llvm::BasicBlock* b);
  llvm::Value* AddI32(llvm::Value* v1, llvm::Value* v2);
  llvm::Value* MulI32(llvm::Value* v1, llvm::Value* v2);
  llvm::Value* DivI32(llvm::Value* v1, llvm::Value* v2);
  llvm::Value* SubI32(llvm::Value* v1, llvm::Value* v2);
  llvm::Value* AddI64(llvm::Value* v1, llvm::Value* v2);
  llvm::Value* MulI64(llvm::Value* v1, llvm::Value* v2);
  llvm::Value* DivI64(llvm::Value* v1, llvm::Value* v2);
  llvm::Value* SubI64(llvm::Value* v1, llvm::Value* v2);
  llvm::Value* CmpI(llvm::CmpInst::Predicate ped, llvm::Value* v1,
                    llvm::Value* v2);
  llvm::Value* AddF(llvm::Value* v1, llvm::Value* v2);
  llvm::Value* MulF(llvm::Value* v1, llvm::Value* v2);
  llvm::Value* DivF(llvm::Value* v1, llvm::Value* v2);
  llvm::Value* SubF(llvm::Value* v1, llvm::Value* v2);
  llvm::Value* CmpF(llvm::CmpInst::Predicate ped, llvm::Value* v1,
                    llvm::Value* v2);
  void Branch(llvm::BasicBlock* b);
  void Branch(llvm::BasicBlock* b, llvm::Value* cond);

 private:
  std::unique_ptr<llvm::LLVMContext> context_;
  std::unique_ptr<llvm::Module> module_;
  std::unique_ptr<llvm::IRBuilder<>> builder_;
};

}  // namespace kush::compile