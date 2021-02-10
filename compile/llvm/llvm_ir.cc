#include "compile/llvm/llvm_ir.h"

#include <system_error>

#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

namespace kush::compile::ir {

LLVMIr::LLVMIr()
    : context_(std::make_unique<llvm::LLVMContext>()),
      module_(std::make_unique<llvm::Module>("query", *context_)),
      builder_(std::make_unique<llvm::IRBuilder<>>(*context_)) {
  auto malloc_type = llvm::FunctionType::get(builder_->getInt8PtrTy(),
                                             builder_->getInt64Ty(), false);
  malloc = llvm::Function::Create(
      malloc_type, llvm::GlobalValue::LinkageTypes::ExternalLinkage, "malloc",
      module_.get());

  auto free_type = llvm::FunctionType::get(builder_->getVoidTy(),
                                           builder_->getInt8PtrTy(), false);
  free = llvm::Function::Create(
      free_type, llvm::GlobalValue::LinkageTypes::ExternalLinkage, "free",
      module_.get());
}

using BasicBlock = LLVMIrTypes::BasicBlock;
using Value = LLVMIrTypes::Value;
using PhiValue = LLVMIrTypes::PhiValue;
using CompType = LLVMIrTypes::CompType;
using Constant = LLVMIrTypes::Constant;
using Function = LLVMIrTypes::Function;
using Type = LLVMIrTypes::Type;

template <typename T>
std::vector<T*> VectorRefToVectorPtr(
    std::vector<std::reference_wrapper<T>>& vec) {
  std::vector<T*> result;
  result.reserve(vec.size());
  for (auto& x : vec) {
    result.push_back(&x.get());
  }
  return result;
}

// Types
Type& LLVMIr::I8Type() { return *builder_->getInt8Ty(); }

Type& LLVMIr::I16Type() { return *builder_->getInt16Ty(); }

Type& LLVMIr::I32Type() { return *builder_->getInt32Ty(); }

Type& LLVMIr::I64Type() { return *builder_->getInt64Ty(); }

Type& LLVMIr::UI32Type() { return *builder_->getInt32Ty(); }

Type& LLVMIr::F64Type() { return *builder_->getDoubleTy(); }

Type& LLVMIr::StructType(std::vector<std::reference_wrapper<Type>> types) {
  return *llvm::StructType::create(*context_, VectorRefToVectorPtr(types));
}

Type& LLVMIr::PointerType(Type& type) {
  return *llvm::PointerType::get(&type, 0);
}

Type& LLVMIr::ArrayType(Type& type) { return *llvm::ArrayType::get(&type, 0); }

Type& LLVMIr::TypeOf(Value& value) { return *value.getType(); }

// Memory
Value& LLVMIr::Malloc(Value& size) { return Call(*malloc, {size}); }

void LLVMIr::Free(Value& ptr) { Call(*free, {ptr}); }

Value& LLVMIr::NullPtr() {
  return *llvm::ConstantPointerNull::get(builder_->getInt8PtrTy());
}

Value& LLVMIr::GetElementPtr(Type& t, Value& ptr,
                             std::vector<std::reference_wrapper<Value>> idx) {
  return *builder_->CreateGEP(&t, &ptr, VectorRefToVectorPtr(idx));
}

Value& LLVMIr::PointerCast(Value& v, Type& t) {
  return *builder_->CreatePointerCast(&v, &t);
}

void LLVMIr::Store(Value& ptr, Value& v) { builder_->CreateStore(&v, &ptr); }

Value& LLVMIr::Load(Value& ptr) { return *builder_->CreateLoad(&ptr); }

void LLVMIr::Memcpy(Value& dest, Value& src, Value& length) {
  auto align = llvm::MaybeAlign(1);
  builder_->CreateMemCpyInline(&dest, align, &src, align, &length);
}

// Function
Function& LLVMIr::CreateFunction(
    Type& result_type, std::vector<std::reference_wrapper<Type>> arg_types) {
  static int i = 0;
  std::string name = "func" + std::to_string(i++);

  auto func_type = llvm::FunctionType::get(
      &result_type, VectorRefToVectorPtr(arg_types), false);

  auto func = llvm::Function::Create(
      func_type, llvm::GlobalValue::LinkageTypes::InternalLinkage, name.c_str(),
      module_.get());
  auto bb = llvm::BasicBlock::Create(*context_, "", func);
  builder_->SetInsertPoint(bb);
  return *func;
}

std::vector<std::reference_wrapper<Value>> GetFunctionArguments(
    Function& func) {
  std::vector<std::reference_wrapper<Value>> result;
  for (auto& arg : func.args()) {
    result.push_back(arg);
  }
  return result;
}

void LLVMIr::Return(Value& v) { builder_->CreateRet(&v); }

std::optional<std::reference_wrapper<Function>> LLVMIr::GetFunction(
    std::string_view name) {
  auto fn = module_->getFunction(name);
  if (fn == nullptr) {
    return std::nullopt;
  }
  return *fn;
}

Value& LLVMIr::Call(Function& func,
                    std::vector<std::reference_wrapper<Value>> arguments) {
  return *builder_->CreateCall(&func, VectorRefToVectorPtr(arguments));
}

// Control Flow
BasicBlock& LLVMIr::GenerateBlock() {
  return *llvm::BasicBlock::Create(*context_, "",
                                   builder_->GetInsertBlock()->getParent());
}

BasicBlock& LLVMIr::CurrentBlock() { return *builder_->GetInsertBlock(); }

void LLVMIr::SetCurrentBlock(BasicBlock& b) { builder_->SetInsertPoint(&b); }

void LLVMIr::Branch(BasicBlock& b) { builder_->CreateBr(&b); }

void LLVMIr::Branch(Value& cond, BasicBlock& b1, BasicBlock& b2) {
  builder_->CreateCondBr(&cond, &b1, &b2);
}

PhiValue& LLVMIr::Phi(Type& type) { return *builder_->CreatePHI(&type, 2); }

void LLVMIr::AddToPhi(PhiValue& phi, Value& v, BasicBlock& b) {
  phi.addIncoming(&v, &b);
}

// I8
Value& LLVMIr::AddI8(Value& v1, Value& v2) {
  return *builder_->CreateAdd(&v1, &v2);
}

Value& LLVMIr::MulI8(Value& v1, Value& v2) {
  return *builder_->CreateMul(&v1, &v2);
}

Value& LLVMIr::DivI8(Value& v1, Value& v2) {
  return *builder_->CreateSDiv(&v1, &v2);
}

Value& LLVMIr::SubI8(Value& v1, Value& v2) {
  return *builder_->CreateSub(&v1, &v2);
}

Value& LLVMIr::CmpI8(CompType cmp, Value& v1, Value& v2) {
  return *builder_->CreateCmp(cmp, &v1, &v2);
}

Value& LLVMIr::LNotI8(Value& v) {
  return *builder_->CreateXor(&v, builder_->getInt8(0));
}

Value& LLVMIr::ConstI8(int8_t v) { return *builder_->getInt8(v); }

// I16
Value& LLVMIr::AddI16(Value& v1, Value& v2) {
  return *builder_->CreateAdd(&v1, &v2);
}

Value& LLVMIr::MulI16(Value& v1, Value& v2) {
  return *builder_->CreateMul(&v1, &v2);
}

Value& LLVMIr::DivI16(Value& v1, Value& v2) {
  return *builder_->CreateSDiv(&v1, &v2);
}

Value& LLVMIr::SubI16(Value& v1, Value& v2) {
  return *builder_->CreateSub(&v1, &v2);
}

Value& LLVMIr::CmpI16(CompType cmp, Value& v1, Value& v2) {
  return *builder_->CreateCmp(cmp, &v1, &v2);
}

Value& LLVMIr::ConstI16(int16_t v) { return *builder_->getInt16(v); }

// I32
Value& LLVMIr::AddI32(Value& v1, Value& v2) {
  return *builder_->CreateAdd(&v1, &v2);
}

Value& LLVMIr::MulI32(Value& v1, Value& v2) {
  return *builder_->CreateMul(&v1, &v2);
}

Value& LLVMIr::DivI32(Value& v1, Value& v2) {
  return *builder_->CreateSDiv(&v1, &v2);
}

Value& LLVMIr::SubI32(Value& v1, Value& v2) {
  return *builder_->CreateSub(&v1, &v2);
}

Value& LLVMIr::CmpI32(CompType cmp, Value& v1, Value& v2) {
  return *builder_->CreateCmp(cmp, &v1, &v2);
}

Value& LLVMIr::ConstI32(int32_t v) { return *builder_->getInt32(v); }

// UI32
Value& LLVMIr::AddUI32(Value& v1, Value& v2) {
  return *builder_->CreateAdd(&v1, &v2);
}

Value& LLVMIr::MulUI32(Value& v1, Value& v2) {
  return *builder_->CreateMul(&v1, &v2);
}

Value& LLVMIr::DivUI32(Value& v1, Value& v2) {
  return *builder_->CreateUDiv(&v1, &v2);
}

Value& LLVMIr::SubUI32(Value& v1, Value& v2) {
  return *builder_->CreateSub(&v1, &v2);
}

Value& LLVMIr::CmpUI32(CompType cmp, Value& v1, Value& v2) {
  return *builder_->CreateCmp(cmp, &v1, &v2);
}

Value& LLVMIr::ConstUI32(uint32_t v) { return *builder_->getInt32(v); }

// I64
Value& LLVMIr::AddI64(Value& v1, Value& v2) {
  return *builder_->CreateAdd(&v1, &v2);
}

Value& LLVMIr::MulI64(Value& v1, Value& v2) {
  return *builder_->CreateMul(&v1, &v2);
}

Value& LLVMIr::DivI64(Value& v1, Value& v2) {
  return *builder_->CreateSDiv(&v1, &v2);
}

Value& LLVMIr::SubI64(Value& v1, Value& v2) {
  return *builder_->CreateSub(&v1, &v2);
}

Value& LLVMIr::CmpI64(CompType cmp, Value& v1, Value& v2) {
  return *builder_->CreateCmp(cmp, &v1, &v2);
}

Value& LLVMIr::ConstI64(int64_t v) { return *builder_->getInt64(v); }

// F64
Value& LLVMIr::AddF64(Value& v1, Value& v2) {
  return *builder_->CreateFAdd(&v1, &v2);
}

Value& LLVMIr::MulF64(Value& v1, Value& v2) {
  return *builder_->CreateFMul(&v1, &v2);
}

Value& LLVMIr::DivF64(Value& v1, Value& v2) {
  return *builder_->CreateFDiv(&v1, &v2);
}

Value& LLVMIr::SubF64(Value& v1, Value& v2) {
  return *builder_->CreateFSub(&v1, &v2);
}

Value& LLVMIr::CmpF64(CompType cmp, Value& v1, Value& v2) {
  return *builder_->CreateFCmp(cmp, &v1, &v2);
}

Value& LLVMIr::ConstF64(double v) {
  return *llvm::ConstantFP::get(builder_->getDoubleTy(), v);
}

void LLVMIr::Compile() const {
  // Write the module to a file
  std::error_code ec;
  llvm::raw_fd_ostream out("/tmp/query.bc", ec);
  llvm::WriteBitcodeToFile(*module_, out);
}

void LLVMIr::Execute() const {}

}  // namespace kush::compile::ir