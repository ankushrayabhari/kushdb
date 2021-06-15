#pragma once

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "compile/program.h"
#include "khir/program_builder.h"
#include "khir/type_manager.h"

namespace kush::khir {

class LLVMBackend : public Backend,
                    public TypeTranslator,
                    public compile::Program {
 public:
  LLVMBackend();
  virtual ~LLVMBackend() = default;

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

  // Globals
  void Translate(const TypeManager& manager,
                 const std::vector<uint64_t>& i64_constants,
                 const std::vector<double>& f64_constants,
                 const std::vector<std::string>& char_array_constants,
                 const std::vector<StructConstant>& struct_constants,
                 const std::vector<ArrayConstant>& array_constants,
                 const std::vector<Global>& globals,
                 const std::vector<uint64_t>& constant_instrs,
                 const std::vector<Function>& functions) override;

  // Program
  void Execute() override;

 private:
  llvm::Constant* ConvertConstantInstr(
      uint64_t instr, std::vector<llvm::Constant*>& constant_values,
      const std::vector<uint64_t>& i64_constants,
      const std::vector<double>& f64_constants,
      const std::vector<std::string>& char_array_constants,
      const std::vector<StructConstant>& struct_constants,
      const std::vector<ArrayConstant>& array_constants,
      const std::vector<Global>& globals);

  void TranslateInstr(
      const std::vector<llvm::Value*>& func_args,
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

  std::chrono::time_point<std::chrono::system_clock> start, comp, end;
};

}  // namespace kush::khir