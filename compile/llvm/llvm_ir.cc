#include "compile/llvm/llvm_ir.h"

#include <dlfcn.h>

#include <iostream>
#include <system_error>
#include <type_traits>

#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

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
Type& LLVMIr::VoidType() { return *builder_->getVoidTy(); }

Type& LLVMIr::I1Type() { return *builder_->getInt1Ty(); }

Type& LLVMIr::I8Type() { return *builder_->getInt8Ty(); }

Type& LLVMIr::I16Type() { return *builder_->getInt16Ty(); }

Type& LLVMIr::I32Type() { return *builder_->getInt32Ty(); }

Type& LLVMIr::I64Type() { return *builder_->getInt64Ty(); }

Type& LLVMIr::UI32Type() { return *builder_->getInt32Ty(); }

Type& LLVMIr::F64Type() { return *builder_->getDoubleTy(); }

Type& LLVMIr::StructType(std::vector<std::reference_wrapper<Type>> types,
                         std::string_view name) {
  auto& ty = *llvm::StructType::create(*context_, VectorRefToVectorPtr(types));
  ty.setName(name);
  return ty;
}

Type& LLVMIr::GetStructType(std::string_view name) {
  for (auto* S : module_->getIdentifiedStructTypes()) {
    if (S->getName().equals(name)) {
      return *S;
    }
  }
  throw std::runtime_error("Unknown struct");
}

Type& LLVMIr::PointerType(Type& type) {
  return *llvm::PointerType::get(&type, 0);
}

Type& LLVMIr::ArrayType(Type& type) { return *llvm::ArrayType::get(&type, 0); }

Type& LLVMIr::FunctionType(Type& result,
                           std::vector<std::reference_wrapper<Type>> args) {
  return *llvm::FunctionType::get(&result, VectorRefToVectorPtr(args), false);
}

Type& LLVMIr::TypeOf(Value& value) { return *value.getType(); }

Value& LLVMIr::SizeOf(Type& type) {
  auto& pointer_type = PointerType(type);
  auto& null = NullPtr(pointer_type);
  auto& size_ptr = GetElementPtr(type, null, {ConstI32(1)});
  return PointerCast(size_ptr, I64Type());
}

// Memory
Value& LLVMIr::Alloca(Type& t) { return *builder_->CreateAlloca(&t); }

Value& LLVMIr::NullPtr(Type& t) {
  return *llvm::ConstantPointerNull::get(llvm::dyn_cast<llvm::PointerType>(&t));
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
Function& CreateFunctionImpl(
    llvm::Module& module, llvm::LLVMContext& context,
    llvm::IRBuilder<>& builder, Type& result_type,
    std::vector<std::reference_wrapper<Type>> arg_types, std::string_view name,
    bool external) {
  std::string fn_name(name);
  auto func_type = llvm::FunctionType::get(
      &result_type, VectorRefToVectorPtr(arg_types), false);
  auto func = llvm::Function::Create(
      func_type,
      external ? llvm::GlobalValue::LinkageTypes::ExternalLinkage
               : llvm::GlobalValue::LinkageTypes::InternalLinkage,
      fn_name.c_str(), module);
  auto bb = llvm::BasicBlock::Create(context, "", func);
  builder.SetInsertPoint(bb);
  return *func;
}

Function& LLVMIr::CreateFunction(
    Type& result_type, std::vector<std::reference_wrapper<Type>> arg_types) {
  static int i = 0;
  std::string name = "func" + std::to_string(i++);
  return CreateFunctionImpl(*module_, *context_, *builder_, result_type,
                            std::move(arg_types), name, false);
}

Function& LLVMIr::CreatePublicFunction(
    Type& result_type, std::vector<std::reference_wrapper<Type>> arg_types,
    std::string_view name) {
  return CreateFunctionImpl(*module_, *context_, *builder_, result_type,
                            std::move(arg_types), name, true);
}

Function& LLVMIr::DeclareExternalFunction(
    std::string_view name, Type& result_type,
    std::vector<std::reference_wrapper<Type>> arg_types) {
  std::string name_to_create(name);
  auto func_type = llvm::FunctionType::get(
      &result_type, VectorRefToVectorPtr(arg_types), false);
  auto func = llvm::Function::Create(
      func_type, llvm::GlobalValue::LinkageTypes::ExternalLinkage,
      name_to_create.c_str(), module_.get());
  return *func;
}

Function& LLVMIr::GetFunction(std::string_view name) {
  return *module_->getFunction(name);
}

std::vector<std::reference_wrapper<Value>> LLVMIr::GetFunctionArguments(
    Function& func) {
  std::vector<std::reference_wrapper<Value>> result;
  for (auto& arg : func.args()) {
    result.push_back(arg);
  }
  return result;
}

void LLVMIr::Return(Value& v) { builder_->CreateRet(&v); }

void LLVMIr::Return() { builder_->CreateRetVoid(); }

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

bool LLVMIr::IsTerminated(BasicBlock& b) {
  return b.getTerminator() != nullptr;
}

void LLVMIr::SetCurrentBlock(BasicBlock& b) { builder_->SetInsertPoint(&b); }

void LLVMIr::Branch(BasicBlock& b) { builder_->CreateBr(&b); }

void LLVMIr::Branch(Value& cond, BasicBlock& b1, BasicBlock& b2) {
  builder_->CreateCondBr(&cond, &b1, &b2);
}

PhiValue& LLVMIr::Phi(Type& type) { return *builder_->CreatePHI(&type, 2); }

void LLVMIr::AddToPhi(PhiValue& phi, Value& v, BasicBlock& b) {
  phi.addIncoming(&v, &b);
}

// I1
Value& LLVMIr::LNotI1(Value& v) {
  return *builder_->CreateXor(&v, builder_->getInt1(true));
}

Value& LLVMIr::CmpI1(CompType cmp, Value& v1, Value& v2) {
  return *builder_->CreateCmp(cmp, &v1, &v2);
}

Value& LLVMIr::ConstI1(bool v) { return *builder_->getInt1(v); }

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

Value& LLVMIr::ZextI64(Value& v) {
  return *builder_->CreateZExt(&v, builder_->getInt64Ty());
}

Value& LLVMIr::F64ConversionI64(Value& v) {
  return *builder_->CreateFPToUI(&v, builder_->getInt64Ty());
}

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

Value& LLVMIr::CastSignedIntToF64(Value& v) {
  return *builder_->CreateCast(llvm::Instruction::SIToFP, &v,
                               builder_->getDoubleTy());
}

// Globals
Value& LLVMIr::ConstString(std::string_view s) {
  return *builder_->CreateGlobalStringPtr(s);
}

Value& LLVMIr::ConstStruct(Type& t,
                           std::vector<std::reference_wrapper<Value>> v) {
  std::vector<llvm::Constant*> constants;
  constants.reserve(v.size());
  for (auto& x : v) {
    constants.push_back(llvm::dyn_cast<llvm::Constant>(&x.get()));
  }

  auto* st = llvm::dyn_cast<llvm::StructType>(&t);

  auto const_struct = llvm::ConstantStruct::get(st, constants);

  auto ptr = new llvm::GlobalVariable(
      *module_, &t, true, llvm::GlobalValue::LinkageTypes::InternalLinkage,
      const_struct);
  return *ptr;
}

void LLVMIr::Compile() const {
  gen = std::chrono::system_clock::now();

  // Write the module to a file
  std::error_code ec;
  llvm::raw_fd_ostream out("/tmp/query.bc", ec);
  llvm::WriteBitcodeToFile(*module_, out);
  out.close();

  if (system("opt -O3 < /tmp/query.bc > /tmp/query_opt.bc")) {
    throw std::runtime_error("Failed to optimize module.");
  }

  if (system("llc -relocation-model=pic -filetype=obj /tmp/query_opt.bc -o "
             "/tmp/query.o")) {
    throw std::runtime_error("Failed to compile file.");
  }

  if (system("clang++ -flto=thin -shared -fpic bazel-bin/util/libprint_util.so "
             "bazel-bin/data/libstring.so bazel-bin/data/libcolumn_data.so "
             "bazel-bin/data/libvector.so bazel-bin/data/libhash_table.so "
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