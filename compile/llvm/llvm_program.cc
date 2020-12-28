#include "compile/llvm/llvm_program.h"

#include <memory>

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

namespace kush::compile {

void LLVMProgram::CodegenInitialize() {
  llvm_context_ = std::make_unique<llvm::LLVMContext>();
  module_ = std::make_unique<llvm::Module>("query", *llvm_context_);
  builder_ = std::make_unique<llvm::IRBuilder<>>(*llvm_context_);
}

void LLVMProgram::CodegenFinalize() {}

void LLVMProgram::Compile() {}

void LLVMProgram::Execute() {}

llvm::IRBuilder<>& LLVMProgram::Builder() { return *builder_; }

}  // namespace kush::compile