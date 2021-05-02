#include "compile/llvm/llvm_ir.h"

#include <dlfcn.h>
#include <iostream>
#include <system_error>
#include <type_traits>

#include "absl/types/span.h"

#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"

#include "util/nn.h"

namespace kush::compile::ir {

LLVMIr::LLVMIr()
    : context_(std::make_unique<llvm::LLVMContext>()),
      module_(std::make_unique<llvm::Module>("query", *context_)),
      builder_(std::make_unique<llvm::IRBuilder<>>(*context_)) {
  start = std::chrono::system_clock::now();
}

using BasicBlock = LLVMIrTypes::BasicBlock;
using Value = LLVMIrTypes::Value;
using PhiValue = LLVMIrTypes::PhiValue;
using CompType = LLVMIrTypes::CompType;
using Function = LLVMIrTypes::Function;
using Type = LLVMIrTypes::Type;

template <typename T>
std::vector<T*> NNVectorToVectorPtr(absl::Span<const util::nn<T*>>& vec) {
  std::vector<T*> result;
  result.reserve(vec.size());
  for (auto x : vec) {
    result.push_back(x);
  }
  return result;
}

// Types
Type LLVMIr::VoidType() { return NN_CHECK_THROW(builder_->getVoidTy()); }

Type LLVMIr::I1Type() { return NN_CHECK_THROW(builder_->getInt1Ty()); }

Type LLVMIr::I8Type() { return NN_CHECK_THROW(builder_->getInt8Ty()); }

Type LLVMIr::I16Type() { return NN_CHECK_THROW(builder_->getInt16Ty()); }

Type LLVMIr::I32Type() { return NN_CHECK_THROW(builder_->getInt32Ty()); }

Type LLVMIr::I64Type() { return NN_CHECK_THROW(builder_->getInt64Ty()); }

Type LLVMIr::F64Type() { return NN_CHECK_THROW(builder_->getDoubleTy()); }

Type LLVMIr::StructType(absl::Span<const Type> types, std::string_view name) {
  auto ty = NN_CHECK_THROW(
      llvm::StructType::create(*context_, NNVectorToVectorPtr(types)));
  ty->setName(name);
  return ty;
}

Type LLVMIr::GetStructType(std::string_view name) {
  for (auto* S : module_->getIdentifiedStructTypes()) {
    if (S->getName().equals(name)) {
      return NN_CHECK_THROW(S);
    }
  }
  throw std::runtime_error("Unknown struct");
}

Type LLVMIr::PointerType(Type type) {
  return NN_CHECK_THROW(llvm::PointerType::get(type, 0));
}

Type LLVMIr::ArrayType(Type type, int len) {
  return NN_CHECK_THROW(llvm::ArrayType::get(type, len));
}

Type LLVMIr::FunctionType(Type result, absl::Span<const Type> args) {
  return NN_CHECK_THROW(
      llvm::FunctionType::get(result, NNVectorToVectorPtr(args), false));
}

Type LLVMIr::TypeOf(Value value) { return NN_CHECK_THROW(value->getType()); }

Value LLVMIr::SizeOf(Type type) {
  auto pointer_type = PointerType(type);
  auto null = NullPtr(pointer_type);
  auto size_ptr = GetElementPtr(type, null, {ConstI32(1)});
  return PointerCast(size_ptr, I64Type());
}

// Memory
Value LLVMIr::Alloca(Type t) {
  return NN_CHECK_THROW(builder_->CreateAlloca(t));
}

Value LLVMIr::NullPtr(Type t) {
  return NN_CHECK_THROW(llvm::ConstantPointerNull::get(
      llvm::dyn_cast<llvm::PointerType>(t.as_nullable())));
}

Value LLVMIr::GetElementPtr(Type t, Value ptr, absl::Span<const Value> idx) {
  return NN_CHECK_THROW(builder_->CreateGEP(t, ptr, NNVectorToVectorPtr(idx)));
}

Value LLVMIr::PointerCast(Value v, Type t) {
  return NN_CHECK_THROW(builder_->CreatePointerCast(v, t));
}

void LLVMIr::Store(Value ptr, Value v) {
  NN_CHECK_THROW(builder_->CreateStore(v, ptr));
}

Value LLVMIr::Load(Value ptr) {
  return NN_CHECK_THROW(builder_->CreateLoad(ptr));
}

void LLVMIr::Memcpy(Value dest, Value src, Value length) {
  auto align = llvm::MaybeAlign(1);
  builder_->CreateMemCpyInline(dest, align, src, align, length);
}

// Function
Function CreateFunctionImpl(llvm::Module& module, llvm::LLVMContext& context,
                            llvm::IRBuilder<>& builder, Type result_type,
                            absl::Span<const Type> arg_types,
                            std::string_view name, bool external) {
  std::string fn_name(name);
  auto func_type = llvm::FunctionType::get(
      result_type, NNVectorToVectorPtr(arg_types), false);
  auto func = NN_CHECK_THROW(llvm::Function::Create(
      func_type,
      external ? llvm::GlobalValue::LinkageTypes::ExternalLinkage
               : llvm::GlobalValue::LinkageTypes::InternalLinkage,
      fn_name.c_str(), module));
  auto bb = llvm::BasicBlock::Create(context, "", func);
  builder.SetInsertPoint(bb);
  return func;
}

Function LLVMIr::CreateFunction(Type result_type,
                                absl::Span<const Type> arg_types) {
  static int i = 0;
  std::string name = "func" + std::to_string(i++);
  return CreateFunctionImpl(*module_, *context_, *builder_, result_type,
                            std::move(arg_types), name, false);
}

Function LLVMIr::CreatePublicFunction(Type result_type,
                                      absl::Span<const Type> arg_types,
                                      std::string_view name) {
  return CreateFunctionImpl(*module_, *context_, *builder_, result_type,
                            std::move(arg_types), name, true);
}

Function LLVMIr::DeclareExternalFunction(std::string_view name,
                                         Type result_type,
                                         absl::Span<const Type> arg_types) {
  std::string name_to_create(name);
  auto func_type = llvm::FunctionType::get(
      result_type, NNVectorToVectorPtr(arg_types), false);
  auto func = llvm::Function::Create(
      func_type, llvm::GlobalValue::LinkageTypes::ExternalLinkage,
      name_to_create.c_str(), module_.get());
  return NN_CHECK_THROW(func);
}

Function LLVMIr::GetFunction(std::string_view name) {
  return NN_CHECK_THROW(module_->getFunction(name));
}

std::vector<Value> LLVMIr::GetFunctionArguments(Function func) {
  std::vector<Value> result;
  for (auto& arg : func->args()) {
    result.push_back(NN_CHECK_THROW(&arg));
  }
  return result;
}

void LLVMIr::Return(Value v) { builder_->CreateRet(v); }

void LLVMIr::Return() { builder_->CreateRetVoid(); }

Value LLVMIr::Call(Function func, absl::Span<const Value> arguments) {
  return NN_CHECK_THROW(
      builder_->CreateCall(func.as_nullable(), NNVectorToVectorPtr(arguments)));
}

Value LLVMIr::Call(Value func, Type function_type,
                   absl::Span<const Value> arguments) {
  return NN_CHECK_THROW(llvm::CallInst::Create(
      llvm::dyn_cast<llvm::FunctionType>(function_type.as_nullable()), func,
      NNVectorToVectorPtr(arguments), "", builder_->GetInsertBlock()));
}

// Control Flow
BasicBlock LLVMIr::GenerateBlock() {
  return NN_CHECK_THROW(llvm::BasicBlock::Create(
      *context_, "", builder_->GetInsertBlock()->getParent()));
}

BasicBlock LLVMIr::CurrentBlock() {
  return NN_CHECK_THROW(builder_->GetInsertBlock());
}

bool LLVMIr::IsTerminated(BasicBlock b) {
  return b->getTerminator() != nullptr;
}

void LLVMIr::SetCurrentBlock(BasicBlock b) { builder_->SetInsertPoint(b); }

void LLVMIr::Branch(BasicBlock b) { builder_->CreateBr(b); }

void LLVMIr::Branch(Value cond, BasicBlock b1, BasicBlock b2) {
  builder_->CreateCondBr(cond, b1, b2);
}

PhiValue LLVMIr::Phi(Type type) {
  return NN_CHECK_THROW(builder_->CreatePHI(type, 2));
}

void LLVMIr::AddToPhi(PhiValue phi, Value v, BasicBlock b) {
  phi->addIncoming(v, b);
}

// I1
Value LLVMIr::LNotI1(Value v) {
  return NN_CHECK_THROW(builder_->CreateXor(v, builder_->getInt1(true)));
}

Value LLVMIr::CmpI1(CompType cmp, Value v1, Value v2) {
  return NN_CHECK_THROW(builder_->CreateCmp(cmp, v1, v2));
}

Value LLVMIr::ConstI1(bool v) { return NN_CHECK_THROW(builder_->getInt1(v)); }

// I8
Value LLVMIr::AddI8(Value v1, Value v2) {
  return NN_CHECK_THROW(builder_->CreateAdd(v1, v2));
}

Value LLVMIr::MulI8(Value v1, Value v2) {
  return NN_CHECK_THROW(builder_->CreateMul(v1, v2));
}

Value LLVMIr::DivI8(Value v1, Value v2) {
  return NN_CHECK_THROW(builder_->CreateSDiv(v1, v2));
}

Value LLVMIr::SubI8(Value v1, Value v2) {
  return NN_CHECK_THROW(builder_->CreateSub(v1, v2));
}

Value LLVMIr::CmpI8(CompType cmp, Value v1, Value v2) {
  return NN_CHECK_THROW(builder_->CreateCmp(cmp, v1, v2));
}

Value LLVMIr::ConstI8(int8_t v) { return NN_CHECK_THROW(builder_->getInt8(v)); }

// I16
Value LLVMIr::AddI16(Value v1, Value v2) {
  return NN_CHECK_THROW(builder_->CreateAdd(v1, v2));
}

Value LLVMIr::MulI16(Value v1, Value v2) {
  return NN_CHECK_THROW(builder_->CreateMul(v1, v2));
}

Value LLVMIr::DivI16(Value v1, Value v2) {
  return NN_CHECK_THROW(builder_->CreateSDiv(v1, v2));
}

Value LLVMIr::SubI16(Value v1, Value v2) {
  return NN_CHECK_THROW(builder_->CreateSub(v1, v2));
}

Value LLVMIr::CmpI16(CompType cmp, Value v1, Value v2) {
  return NN_CHECK_THROW(builder_->CreateCmp(cmp, v1, v2));
}

Value LLVMIr::ConstI16(int16_t v) {
  return NN_CHECK_THROW(builder_->getInt16(v));
}

// I32
Value LLVMIr::AddI32(Value v1, Value v2) {
  return NN_CHECK_THROW(builder_->CreateAdd(v1, v2));
}

Value LLVMIr::MulI32(Value v1, Value v2) {
  return NN_CHECK_THROW(builder_->CreateMul(v1, v2));
}

Value LLVMIr::DivI32(Value v1, Value v2) {
  return NN_CHECK_THROW(builder_->CreateSDiv(v1, v2));
}

Value LLVMIr::SubI32(Value v1, Value v2) {
  return NN_CHECK_THROW(builder_->CreateSub(v1, v2));
}

Value LLVMIr::CmpI32(CompType cmp, Value v1, Value v2) {
  return NN_CHECK_THROW(builder_->CreateCmp(cmp, v1, v2));
}

Value LLVMIr::ConstI32(int32_t v) {
  return NN_CHECK_THROW(builder_->getInt32(v));
}

// I64
Value LLVMIr::AddI64(Value v1, Value v2) {
  return NN_CHECK_THROW(builder_->CreateAdd(v1, v2));
}

Value LLVMIr::MulI64(Value v1, Value v2) {
  return NN_CHECK_THROW(builder_->CreateMul(v1, v2));
}

Value LLVMIr::DivI64(Value v1, Value v2) {
  return NN_CHECK_THROW(builder_->CreateSDiv(v1, v2));
}

Value LLVMIr::SubI64(Value v1, Value v2) {
  return NN_CHECK_THROW(builder_->CreateSub(v1, v2));
}

Value LLVMIr::CmpI64(CompType cmp, Value v1, Value v2) {
  return NN_CHECK_THROW(builder_->CreateCmp(cmp, v1, v2));
}

Value LLVMIr::ConstI64(int64_t v) {
  return NN_CHECK_THROW(builder_->getInt64(v));
}

Value LLVMIr::ZextI64(Value v) {
  return NN_CHECK_THROW(builder_->CreateZExt(v, builder_->getInt64Ty()));
}

Value LLVMIr::F64ConversionI64(Value v) {
  return NN_CHECK_THROW(builder_->CreateFPToUI(v, builder_->getInt64Ty()));
}

// F64
Value LLVMIr::AddF64(Value v1, Value v2) {
  return NN_CHECK_THROW(builder_->CreateFAdd(v1, v2));
}

Value LLVMIr::MulF64(Value v1, Value v2) {
  return NN_CHECK_THROW(builder_->CreateFMul(v1, v2));
}

Value LLVMIr::DivF64(Value v1, Value v2) {
  return NN_CHECK_THROW(builder_->CreateFDiv(v1, v2));
}

Value LLVMIr::SubF64(Value v1, Value v2) {
  return NN_CHECK_THROW(builder_->CreateFSub(v1, v2));
}

Value LLVMIr::CmpF64(CompType cmp, Value v1, Value v2) {
  return NN_CHECK_THROW(builder_->CreateFCmp(cmp, v1, v2));
}

Value LLVMIr::ConstF64(double v) {
  return NN_CHECK_THROW(llvm::ConstantFP::get(builder_->getDoubleTy(), v));
}

Value LLVMIr::CastSignedIntToF64(Value v) {
  return NN_CHECK_THROW(builder_->CreateCast(llvm::Instruction::SIToFP, v,
                                             builder_->getDoubleTy()));
}

// Globals
Value LLVMIr::GlobalConstString(std::string_view s) {
  return NN_CHECK_THROW(builder_->CreateGlobalStringPtr(s));
}

Value LLVMIr::GlobalStruct(bool constant, Type t,
                           absl::Span<const Value> value) {
  std::vector<llvm::Constant*> constants;
  constants.reserve(value.size());
  for (auto x : value) {
    constants.push_back(llvm::dyn_cast<llvm::Constant>(x.as_nullable()));
  }
  auto* st = llvm::dyn_cast<llvm::StructType>(t.as_nullable());
  auto init = llvm::ConstantStruct::get(st, constants);
  auto ptr = new llvm::GlobalVariable(
      *module_, t, constant, llvm::GlobalValue::LinkageTypes::InternalLinkage,
      init);
  return NN_CHECK_THROW(ptr);
}

Value LLVMIr::GlobalArray(bool constant, Type t,
                          absl::Span<const Value> value) {
  std::vector<llvm::Constant*> constants;
  constants.reserve(value.size());
  for (auto& x : value) {
    constants.push_back(llvm::dyn_cast<llvm::Constant>(x.as_nullable()));
  }

  auto* st = llvm::dyn_cast<llvm::ArrayType>(t.as_nullable());

  auto init = llvm::ConstantArray::get(st, constants);

  auto ptr = new llvm::GlobalVariable(
      *module_, t, constant, llvm::GlobalValue::LinkageTypes::InternalLinkage,
      init);
  return NN_CHECK_THROW(ptr);
}

Value LLVMIr::GlobalPointer(bool constant, Type t, Value v) {
  auto ptr = new llvm::GlobalVariable(
      *module_, t, constant, llvm::GlobalValue::LinkageTypes::InternalLinkage,
      llvm::dyn_cast<llvm::Constant>(v.as_nullable()));
  return NN_CHECK_THROW(ptr);
}

void LLVMIr::Compile() const {
  gen = std::chrono::system_clock::now();
  llvm::verifyModule(*module_, &llvm::errs());
  // module_->print(llvm::errs(), nullptr);

  llvm::legacy::PassManager pass;

  // Setup Optimizations
  pass.add(llvm::createInstructionCombiningPass());
  pass.add(llvm::createReassociatePass());
  pass.add(llvm::createGVNPass());
  pass.add(llvm::createCFGSimplificationPass());
  pass.add(llvm::createAggressiveDCEPass());
  pass.add(llvm::createCFGSimplificationPass());

  // Setup Compilation
  LLVMInitializeX86TargetInfo();
  LLVMInitializeX86Target();
  LLVMInitializeX86TargetMC();
  LLVMInitializeX86AsmParser();
  LLVMInitializeX86AsmPrinter();

  auto target_triple = llvm::sys::getDefaultTargetTriple();
  module_->setTargetTriple(target_triple);

  std::string error;
  auto target = llvm::TargetRegistry::lookupTarget(target_triple, error);
  if (!target) {
    throw std::runtime_error("Target not found: " + error);
  }

  auto cpu = "znver3";
  auto features = "";

  llvm::TargetOptions opt;
  auto reloc_model =
      llvm::Optional<llvm::Reloc::Model>(llvm::Reloc::Model::PIC_);
  auto target_machine = target->createTargetMachine(target_triple, cpu,
                                                    features, opt, reloc_model);

  module_->setDataLayout(target_machine->createDataLayout());

  auto file = "/tmp/query.o";
  std::error_code error_code;
  llvm::raw_fd_ostream dest(file, error_code, llvm::sys::fs::OF_None);
  if (error_code) {
    throw std::runtime_error(error_code.message());
  }
  auto FileType = llvm::CGFT_ObjectFile;
  if (target_machine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
    throw std::runtime_error("Cannot emit object file.");
  }

  // Run Optimizations and Compilation
  pass.run(*module_);
  dest.close();

  // Link
  if (system(
          "clang++ -shared -fpic bazel-bin/runtime/libprint_util.so "
          "bazel-bin/runtime/libstring.so bazel-bin/runtime/libcolumn_data.so "
          "bazel-bin/runtime/libvector.so bazel-bin/runtime/libhash_table.so "
          "bazel-bin/runtime/libtuple_idx_table.so "
          "bazel-bin/runtime/libcolumn_index.so "
          "bazel-bin/runtime/libskinner_join_executor.so "
          "/tmp/query.o -o /tmp/query.so")) {
    throw std::runtime_error("Failed to link file.");
  }

  comp = std::chrono::system_clock::now();
}

void LLVMIr::Execute() const {
  void* handle = dlopen("/tmp/query.so", RTLD_LAZY);

  if (!handle) {
    throw std::runtime_error(dlerror());
  }

  using compute_fn = std::add_pointer<void()>::type;
  auto process_query = (compute_fn)(dlsym(handle, "compute"));
  if (!process_query) {
    dlclose(handle);
    throw std::runtime_error("Failed to get compute fn.");
  }
  link = std::chrono::system_clock::now();

  process_query();
  dlclose(handle);

  end = std::chrono::system_clock::now();
  std::cerr << "Performance stats (ms):" << std::endl;
  std::chrono::duration<double, std::milli> elapsed_seconds = gen - start;
  std::cerr << "Code generation: " << elapsed_seconds.count() << std::endl;
  elapsed_seconds = comp - gen;
  std::cerr << "Compilation: " << elapsed_seconds.count() << std::endl;
  elapsed_seconds = link - comp;
  std::cerr << "Linking: " << elapsed_seconds.count() << std::endl;
  elapsed_seconds = end - link;
  std::cerr << "Execution: " << elapsed_seconds.count() << std::endl;
}

}  // namespace kush::compile::ir