#pragma once

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "compile/khir/khir_program_builder.h"
#include "compile/khir/khir_program_translator.h"
#include "compile/khir/type_manager.h"

namespace kush::khir {

class KhirLLVMBackend : public KhirProgramTranslator {
 public:
  KhirLLVMBackend();
  void TranslateVoidType() override;
  void TranslateI1Type() override;
  void TranslateI8Type() override;
  void TranslateI16Type() override;
  void TranslateI32Type() override;
  void TranslateI64Type() override;
  void TranslateF64Type() override;
  void TranslatePointerType(Type elem) override;
  void TranslateArrayType(Type elem, int len) override;
  void TranslateFunctionType(Type result,
                             absl::Span<const Type> arg_types) override;
  void TranslateStructType(absl::Span<const Type> elem_types) override;

 private:
  std::unique_ptr<llvm::LLVMContext> context_;
  std::unique_ptr<llvm::Module> module_;
  std::unique_ptr<llvm::IRBuilder<>> builder_;
  std::vector<llvm::Type*> types_;
};

}  // namespace kush::khir