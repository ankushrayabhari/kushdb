#pragma once

#include <memory>

#include "compile/program.h"
#include "compile/program_builder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

namespace kush::compile::ir {

class LLVMIrTypes {
 public:
  using BasicBlock = llvm::BasicBlock;
  using Value = llvm::Value;
  using CompType = llvm::CmpInst::Predicate;
  using Constant = llvm::Constant;
};

class LLVMIr : public Program, ProgramBuilder<LLVMIrTypes> {
 public:
  LLVMIr();
  ~LLVMIr() = default;

  // Compile
  void Compile() const override;
  void Execute() const override;

  // Control Flow
  BasicBlock& GenerateBlock() override;
  BasicBlock& CurrentBlock() override;
  void SetCurrentBlock(BasicBlock& b) override;
  void Branch(BasicBlock& b) override;
  void Branch(Value& cond, BasicBlock& b1, BasicBlock& b2) override;
  Value& Phi() override;
  void AddToPhi(Value& phi, Value& v, BasicBlock& b) override;

  // I8
  Value& AddI8(Value& v1, Value& v2) override;
  Value& MulI8(Value& v1, Value& v2) override;
  Value& DivI8(Value& v1, Value& v2) override;
  Value& SubI8(Value& v1, Value& v2) override;
  Value& CmpI8(CompType cmp, Value& v1, Value& v2) override;
  Value& LNotI8(Value& v) override;
  Value& ConstI8(int8_t v) override;

  // I16
  Value& AddI16(Value& v1, Value& v2) override;
  Value& MulI16(Value& v1, Value& v2) override;
  Value& DivI16(Value& v1, Value& v2) override;
  Value& SubI16(Value& v1, Value& v2) override;
  Value& CmpI16(CompType cmp, Value& v1, Value& v2) override;
  Value& ConstI16(int16_t v) override;

  // I32
  Value& AddI32(Value& v1, Value& v2) override;
  Value& MulI32(Value& v1, Value& v2) override;
  Value& DivI32(Value& v1, Value& v2) override;
  Value& SubI32(Value& v1, Value& v2) override;
  Value& CmpI32(CompType cmp, Value& v1, Value& v2) override;
  Value& ConstI32(int32_t v) override;

  // UI32
  Value& AddUI32(Value& v1, Value& v2) override;
  Value& SubUI32(Value& v1, Value& v2) override;
  Value& CmpUI32(CompType cmp, Value& v1, Value& v2) override;
  Value& ConstUI32(uint32_t v) override;

  // I64
  Value& AddI64(Value& v1, Value& v2) override;
  Value& MulI64(Value& v1, Value& v2) override;
  Value& DivI64(Value& v1, Value& v2) override;
  Value& SubI64(Value& v1, Value& v2) override;
  Value& CmpI64(CompType cmp, Value& v1, Value& v2) override;
  Value& ConstI64(int64_t v) override;

  // F64
  Value& AddF64(Value& v1, Value& v2) override;
  Value& MulF64(Value& v1, Value& v2) override;
  Value& DivF64(Value& v1, Value& v2) override;
  Value& SubF64(Value& v1, Value& v2) override;
  Value& CmpF64(CompType cmp, Value& v1, Value& v2) override;
  Value& ConstF64(double v) override;

 private:
  std::unique_ptr<llvm::LLVMContext> context_;
  std::unique_ptr<llvm::Module> module_;
  std::unique_ptr<llvm::IRBuilder<>> builder_;
};

}  // namespace kush::compile::ir