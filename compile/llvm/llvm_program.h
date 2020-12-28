#pragma once

#include <memory>

#include "compile/program.h"
#include "llvm/include/llvm/IR/IRBuilder.h"
#include "llvm/include/llvm/IR/LLVMContext.h"
#include "llvm/include/llvm/IR/Module.h"

namespace kush::compile {

class LLVMProgram : public Program {
 public:
  void CodegenInitialize();
  void CodegenFinalize();
  void Compile() override;
  void Execute() override;

 private:
  std::unique_ptr<llvm::LLVMContext> llvm_context_;
  std::unique_ptr<llvm::Module> module_;
  std::unique_ptr<llvm::IRBuilder<>> builder_;
};

}  // namespace kush::compile