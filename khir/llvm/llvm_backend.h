#pragma once

#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "khir/backend.h"
#include "khir/program.h"
#include "khir/type_manager.h"

namespace kush::khir {

class LLVMBackend : public Backend, public TypeTranslator {
 public:
  LLVMBackend();
  virtual ~LLVMBackend();

  // Types
  void TranslateOpaqueType(std::string_view name) override;
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

  // Program
  void Translate(const Program& program) override;

  // Backend
  void Compile() override;
  void* GetFunction(std::string_view name) const override;

 private:
  llvm::Constant* ConvertConstantInstr(
      uint64_t instr, std::vector<llvm::Constant*>& constant_values,
      const Program& program);

  void TranslateInstr(
      const TypeManager& manager, const std::vector<llvm::Value*>& func_args,
      const std::vector<llvm::BasicBlock*>& basic_blocks,
      std::vector<llvm::Value*>& values,
      const std::vector<llvm::Constant*>& constant_values,
      absl::flat_hash_map<
          uint32_t, std::vector<std::pair<llvm::Value*, llvm::BasicBlock*>>>&
          phi_member_list,
      const std::vector<uint64_t>& instructions, int instr_idx);

  std::unique_ptr<llvm::LLVMContext> context_;
  std::unique_ptr<llvm::Module> module_;
  std::unique_ptr<llvm::IRBuilder<>> builder_;
  std::vector<llvm::Type*> types_;
  std::vector<llvm::Function*> functions_;
  std::vector<llvm::Value*> call_args_;

  std::vector<std::tuple<std::string, void*>> external_functions_;
  std::vector<std::string> public_functions_;

  std::unique_ptr<llvm::orc::LLJIT> jit_;

  std::string name_;

  absl::flat_hash_map<std::string, void*> compiled_fn_;
};

}  // namespace kush::khir