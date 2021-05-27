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

  // Types
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

  void TranslateFuncDecl(bool external, std::string_view name,
                         Type function_type) override;
  void TranslateFuncBody(int func_idx,
                         const std::vector<uint64_t>& i64_constants,
                         const std::vector<double>& f64_constants,
                         const std::vector<int>& basic_block_order,
                         const std::vector<std::pair<int, int>>& basic_blocks,
                         const std::vector<uint64_t>& instructions) override;

 private:
  void TranslateInstr(
      const std::vector<llvm::Value*>& func_args,
      const std::vector<uint64_t>& i64_constants,
      const std::vector<double>& f64_constants,
      std::vector<llvm::Value*>& values,
      absl::flat_hash_map<
          uint32_t, std::vector<std::pair<llvm::Value*, llvm::BasicBlock*>>>&
          phi_member_list,
      const std::vector<uint64_t>& instructions, int instr_idx);

  std::unique_ptr<llvm::LLVMContext> context_;
  std::unique_ptr<llvm::Module> module_;
  std::unique_ptr<llvm::IRBuilder<>> builder_;
  std::vector<llvm::Type*> types_;
  std::vector<llvm::Function*> functions_;
  std::vector<llvm::BasicBlock*> basic_blocks_;
  std::vector<llvm::Value*> call_args_;
};

}  // namespace kush::khir