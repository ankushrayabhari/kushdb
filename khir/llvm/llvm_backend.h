#pragma once

#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "khir/backend.h"
#include "khir/program.h"
#include "khir/type_manager.h"

namespace kush::khir {

class LLVMTypeManager : public TypeTranslator {
 public:
  LLVMTypeManager(llvm::LLVMContext* c, llvm::IRBuilder<>* b);
  void TranslateOpaqueType(std::string_view name) override;
  void TranslateVoidType() override;
  void TranslateI1Type() override;
  void TranslateI8Type() override;
  void TranslateI16Type() override;
  void TranslateI32Type() override;
  void TranslateI64Type() override;
  void TranslateF64Type() override;
  void TranslateI1Vec8Type() override;
  void TranslateI32Vec8Type() override;
  void TranslatePointerType(Type elem) override;
  void TranslateArrayType(Type elem, int len) override;
  void TranslateFunctionType(Type result,
                             absl::Span<const Type> arg_types) override;
  void TranslateStructType(absl::Span<const Type> elem_types) override;

  const std::vector<llvm::Type*>& GetTypes();

 private:
  std::vector<llvm::Type*> GetTypeArray(
      absl::Span<const Type> elem_types) const;
  llvm::LLVMContext* context_;
  llvm::IRBuilder<>* builder_;
  std::vector<llvm::Type*> types_;
};

class LLVMBackend : public Backend {
 public:
  LLVMBackend(const khir::Program& program);
  virtual ~LLVMBackend() = default;

  // Backend
  void Compile();
  void* GetFunction(std::string_view name) override;

 private:
  struct Fragment {
    std::unique_ptr<llvm::Module> mod;
    std::unique_ptr<llvm::LLVMContext> context;
  };
  Fragment Translate();
  llvm::Constant* GetConstant(Value v, llvm::Module* mod,
                              llvm::LLVMContext* context,
                              llvm::IRBuilder<>* builder,
                              const std::vector<llvm::Type*>& types,
                              std::vector<llvm::Constant*>& constant_values);
  llvm::Value* GetValue(khir::Value v,
                        std::vector<llvm::Constant*>& constant_values,
                        const std::vector<llvm::Value*>& values,
                        llvm::Module* mod, llvm::LLVMContext* context,
                        llvm::IRBuilder<>* builder,
                        const std::vector<llvm::Type*>& types);

  void TranslateInstr(
      int instr_idx, const std::vector<uint64_t>& instructions, llvm::Module* m,
      llvm::LLVMContext* c, llvm::IRBuilder<>* b,
      const std::vector<llvm::Type*>& types,
      const std::vector<llvm::Value*>& func_args,
      const std::vector<llvm::BasicBlock*>& basic_blocks,
      std::vector<llvm::Value*>& values, std::vector<llvm::Value*>& call_args,
      std::vector<llvm::Constant*>& constant_values,
      absl::flat_hash_map<
          uint32_t, std::vector<std::pair<llvm::Value*, llvm::BasicBlock*>>>&
          phi_member_list);

  const khir::Program& program_;
  std::unique_ptr<llvm::orc::LLJIT> jit_;
  absl::flat_hash_map<std::string, void*> compiled_fn_;
};

}  // namespace kush::khir