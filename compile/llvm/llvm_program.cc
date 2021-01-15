#include "compile/llvm/llvm_program.h"

#include <system_error>

#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

namespace kush::compile {

LLVMProgram::LLVMProgram()
    : context_(std::make_unique<llvm::LLVMContext>()),
      module_(std::make_unique<llvm::Module>("query", *context_)),
      builder_(std::make_unique<llvm::IRBuilder<>>(*context_)) {}

void LLVMProgram::Compile() const {
  // Write the module to a file
  std::error_code ec;
  llvm::raw_fd_ostream out("/tmp/query.bc", ec);
  llvm::WriteBitcodeToFile(*module_, out);
}

void LLVMProgram::Execute() const {}

}  // namespace kush::compile