#pragma once

#include <memory>

#include "absl/types/span.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "compile/program.h"
#include "compile/program_builder.h"
#include "util/nn.h"

namespace kush::compile::ir {

class LLVMIrTypes {
 public:
  using BasicBlock = util::nn<llvm::BasicBlock*>;
  using Value = util::nn<llvm::Value*>;
  using CompType = llvm::CmpInst::Predicate;
  using Function = util::nn<llvm::Function*>;
  using Type = util::nn<llvm::Type*>;
};

class LLVMIr : public Program, public ProgramBuilder<LLVMIrTypes> {
 public:
  LLVMIr();
  ~LLVMIr() = default;

  // Compile
  void Compile() const override;
  void Execute() const override;

  // Types
  Type VoidType() override;
  Type I1Type() override;
  Type I8Type() override;
  Type I16Type() override;
  Type I32Type() override;
  Type I64Type() override;
  Type F64Type() override;
  Type StructType(absl::Span<const Type> types, std::string_view name) override;
  Type GetStructType(std::string_view name) override;
  Type PointerType(Type type) override;
  Type ArrayType(Type type, int len = 0) override;
  Type FunctionType(Type result, absl::Span<const Type> args) override;
  Type TypeOf(Value value) override;
  Value SizeOf(Type type) override;

  // Memory
  Value Alloca(Type t) override;
  Value NullPtr(Type t) override;
  Value GetElementPtr(Type t, Value ptr,
                      absl::Span<const int32_t> idx) override;
  Value PointerCast(Value v, Type t) override;
  void Store(Value ptr, Value v) override;
  Value Load(Value ptr) override;
  void Memcpy(Value dest, Value src, Value length) override;

  // Function
  Function CreateFunction(Type result_type,
                          absl::Span<const Type> arg_types) override;
  Function CreatePublicFunction(Type result_type,
                                absl::Span<const Type> arg_types,
                                std::string_view name) override;
  Function DeclareExternalFunction(std::string_view name, Type result_type,
                                   absl::Span<const Type> arg_types) override;
  Function GetFunction(std::string_view name) override;
  std::vector<Value> GetFunctionArguments(Function func) override;
  void Return(Value v) override;
  void Return() override;
  Value Call(Function name, absl::Span<const Value> arguments) override;
  Value Call(Value func, Type function_type,
             absl::Span<const Value> arguments) override;

  // Control Flow
  BasicBlock GenerateBlock() override;
  BasicBlock CurrentBlock() override;
  bool IsTerminated(BasicBlock b) override;
  void SetCurrentBlock(BasicBlock b) override;
  void Branch(BasicBlock b) override;
  void Branch(Value cond, BasicBlock b1, BasicBlock b2) override;
  Value Phi(Type type) override;
  void AddToPhi(Value phi, Value v, BasicBlock b) override;

  // I1
  Value LNotI1(Value v) override;
  Value CmpI1(CompType cmp, Value v1, Value v2) override;
  Value ConstI1(bool v) override;

  // I8
  Value AddI8(Value v1, Value v2) override;
  Value MulI8(Value v1, Value v2) override;
  Value DivI8(Value v1, Value v2) override;
  Value SubI8(Value v1, Value v2) override;
  Value CmpI8(CompType cmp, Value v1, Value v2) override;
  Value ConstI8(int8_t v) override;

  // I16
  Value AddI16(Value v1, Value v2) override;
  Value MulI16(Value v1, Value v2) override;
  Value DivI16(Value v1, Value v2) override;
  Value SubI16(Value v1, Value v2) override;
  Value CmpI16(CompType cmp, Value v1, Value v2) override;
  Value ConstI16(int16_t v) override;

  // I32
  Value AddI32(Value v1, Value v2) override;
  Value MulI32(Value v1, Value v2) override;
  Value DivI32(Value v1, Value v2) override;
  Value SubI32(Value v1, Value v2) override;
  Value CmpI32(CompType cmp, Value v1, Value v2) override;
  Value ConstI32(int32_t v) override;

  // I64
  Value AddI64(Value v1, Value v2) override;
  Value MulI64(Value v1, Value v2) override;
  Value DivI64(Value v1, Value v2) override;
  Value SubI64(Value v1, Value v2) override;
  Value CmpI64(CompType cmp, Value v1, Value v2) override;
  Value ConstI64(int64_t v) override;
  Value ZextI64(Value v) override;
  Value F64ConversionI64(Value v) override;

  // F64
  Value AddF64(Value v1, Value v2) override;
  Value MulF64(Value v1, Value v2) override;
  Value DivF64(Value v1, Value v2) override;
  Value SubF64(Value v1, Value v2) override;
  Value CmpF64(CompType cmp, Value v1, Value v2) override;
  Value ConstF64(double v) override;
  Value CastSignedIntToF64(Value v) override;

  // Globals
  Value GlobalConstString(std::string_view s) override;
  Value GlobalStruct(bool constant, Type t, absl::Span<const Value> v) override;
  Value GlobalArray(bool constant, Type t, absl::Span<const Value> v) override;
  Value GlobalPointer(bool constant, Type t, Value v) override;

 private:
  std::unique_ptr<llvm::LLVMContext> context_;
  std::unique_ptr<llvm::Module> module_;
  std::unique_ptr<llvm::IRBuilder<>> builder_;
  mutable std::chrono::time_point<std::chrono::system_clock> start, gen, comp,
      link, end;
};

}  // namespace kush::compile::ir