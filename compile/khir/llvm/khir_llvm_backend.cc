#include "compile/khir/llvm/khir_llvm_backend.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "compile/khir/khir_program_builder.h"
#include "compile/khir/khir_program_translator.h"
#include "compile/khir/type_manager.h"

namespace kush::khir {

KhirLLVMBackend::KhirLLVMBackend()
    : context_(std::make_unique<llvm::LLVMContext>()),
      module_(std::make_unique<llvm::Module>("query", *context_)),
      builder_(std::make_unique<llvm::IRBuilder<>>(*context_)) {}

void KhirLLVMBackend::TranslateVoidType() {
  types_.push_back(builder_->getVoidTy());
}

void KhirLLVMBackend::TranslateI1Type() {
  types_.push_back(builder_->getInt1Ty());
}

void KhirLLVMBackend::TranslateI8Type() {
  types_.push_back(builder_->getInt8Ty());
}

void KhirLLVMBackend::TranslateI16Type() {
  types_.push_back(builder_->getInt16Ty());
}

void KhirLLVMBackend::TranslateI32Type() {
  types_.push_back(builder_->getInt32Ty());
}

void KhirLLVMBackend::TranslateI64Type() {
  types_.push_back(builder_->getInt64Ty());
}

void KhirLLVMBackend::TranslateF64Type() {
  types_.push_back(builder_->getDoubleTy());
}

void KhirLLVMBackend::TranslatePointerType(Type elem) {
  types_.push_back(llvm::PointerType::get(types_[elem.GetID()], 0));
}

void KhirLLVMBackend::TranslateArrayType(Type elem, int len) {
  types_.push_back(llvm::ArrayType::get(types_[elem.GetID()], len));
}

std::vector<llvm::Type*> GetTypeArray(const std::vector<llvm::Type*>& types,
                                      absl::Span<const Type> elem_types) {
  std::vector<llvm::Type*> result;
  for (auto x : elem_types) {
    result.push_back(types[x.GetID()]);
  }
  return result;
}

void KhirLLVMBackend::TranslateFunctionType(Type result,
                                            absl::Span<const Type> arg_types) {
  types_.push_back(llvm::FunctionType::get(
      types_[result.GetID()], GetTypeArray(types_, arg_types), false));
}

void KhirLLVMBackend::TranslateStructType(absl::Span<const Type> elem_types) {
  types_.push_back(
      llvm::StructType::create(*context_, GetTypeArray(types_, elem_types)));
}

}  // namespace kush::khir