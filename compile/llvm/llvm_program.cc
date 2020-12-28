#include "compile/llvm/llvm_program.h"

#include <memory>

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

namespace kush::compile {

void LLVMProgram::CodegenInitialize() {}

void LLVMProgram::CodegenFinalize() {}

void LLVMProgram::Compile() {}

void LLVMProgram::Execute() {}

}  // namespace kush::compile