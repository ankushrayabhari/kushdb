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
  using PhiValue = llvm::PHINode;
  using CompType = llvm::CmpInst::Predicate;
  using Constant = llvm::Constant;
  using Function = llvm::Function;
  using Type = llvm::Type;
};

class LLVMIr : public Program, public ProgramBuilder<LLVMIrTypes> {
 public:
  LLVMIr();
  ~LLVMIr() = default;

  // Compile
  void Compile() const override;
  void Execute() const override;

  // Types
  Type& VoidType() override;
  Type& I8Type() override;
  Type& I16Type() override;
  Type& I32Type() override;
  Type& I64Type() override;
  Type& UI32Type() override;
  Type& F64Type() override;
  Type& StructType(std::vector<std::reference_wrapper<Type>> types) override;
  Type& PointerType(Type& type) override;
  Type& ArrayType(Type& type) override;
  Type& TypeOf(Value& value) override;
  Value& SizeOf(Type& type) override;

  // Memory
  Value& Alloca(Type& t) override;
  Value& NullPtr(Type& t) override;
  Value& GetElementPtr(Type& t, Value& ptr,
                       std::vector<std::reference_wrapper<Value>> idx) override;
  Value& PointerCast(Value& v, Type& t) override;
  void Store(Value& ptr, Value& v) override;
  Value& Load(Value& ptr) override;
  void Memcpy(Value& dest, Value& src, Value& length) override;

  // Function
  Function& CreateFunction(
      Type& result_type,
      std::vector<std::reference_wrapper<Type>> arg_types) override;
  Function& CreateExternalFunction(
      Type& result_type, std::vector<std::reference_wrapper<Type>> arg_types,
      std::string_view name) override;
  Function& DeclareExternalFunction(
      std::string_view name, Type& result_type,
      std::vector<std::reference_wrapper<Type>> arg_types) override;
  std::vector<std::reference_wrapper<Value>> GetFunctionArguments(
      Function& func) override;
  void Return(Value& v) override;
  void Return() override;
  Value& Call(Function& name,
              std::vector<std::reference_wrapper<Value>> arguments) override;

  // Control Flow
  BasicBlock& GenerateBlock() override;
  BasicBlock& CurrentBlock() override;
  void SetCurrentBlock(BasicBlock& b) override;
  void Branch(BasicBlock& b) override;
  void Branch(Value& cond, BasicBlock& b1, BasicBlock& b2) override;
  PhiValue& Phi(Type& type) override;
  void AddToPhi(PhiValue& phi, Value& v, BasicBlock& b) override;

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
  Value& MulUI32(Value& v1, Value& v2) override;
  Value& DivUI32(Value& v1, Value& v2) override;
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

  // Globals
  Value& CreateGlobal(std::string_view s) override;

 private:
  std::unique_ptr<llvm::LLVMContext> context_;
  std::unique_ptr<llvm::Module> module_;
  std::unique_ptr<llvm::IRBuilder<>> builder_;
};

}  // namespace kush::compile::ir