#include "khir/llvm/llvm_backend.h"

#include <chrono>
#include <dlfcn.h>
#include <iostream>
#include <system_error>
#include <type_traits>

#include "absl/flags/flag.h"
#include "absl/types/span.h"

#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/ObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/IntrinsicsX86.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"

#include "khir/backend.h"
#include "khir/instruction.h"
#include "khir/llvm/perf_jit_event_listener.h"
#include "khir/type_manager.h"
#include "util/permute.h"

namespace kush::khir {

LLVMTypeManager::LLVMTypeManager(llvm::LLVMContext* c, llvm::IRBuilder<>* b)
    : context_(c), builder_(b) {}

void LLVMTypeManager::TranslateOpaqueType(std::string_view name) {
  types_.push_back(llvm::StructType::create(*context_, name));
}

void LLVMTypeManager::TranslateVoidType() {
  types_.push_back(builder_->getVoidTy());
}

void LLVMTypeManager::TranslateI1Type() {
  types_.push_back(builder_->getInt1Ty());
}

void LLVMTypeManager::TranslateI1Vec8Type() {
  types_.push_back(llvm::VectorType::get(builder_->getInt1Ty(), 8, false));
}

void LLVMTypeManager::TranslateI8Type() {
  types_.push_back(builder_->getInt8Ty());
}

void LLVMTypeManager::TranslateI16Type() {
  types_.push_back(builder_->getInt16Ty());
}

void LLVMTypeManager::TranslateI32Type() {
  types_.push_back(builder_->getInt32Ty());
}

void LLVMTypeManager::TranslateI32Vec8Type() {
  types_.push_back(llvm::FixedVectorType::get(builder_->getInt32Ty(), 8));
}

void LLVMTypeManager::TranslateI64Type() {
  types_.push_back(builder_->getInt64Ty());
}

void LLVMTypeManager::TranslateF64Type() {
  types_.push_back(builder_->getDoubleTy());
}

void LLVMTypeManager::TranslatePointerType(Type elem) {
  types_.push_back(llvm::PointerType::get(types_[elem.GetID()], 0));
}

void LLVMTypeManager::TranslateArrayType(Type elem, int len) {
  types_.push_back(llvm::ArrayType::get(types_[elem.GetID()], len));
}

std::vector<llvm::Type*> LLVMTypeManager::GetTypeArray(
    absl::Span<const Type> elem_types) const {
  std::vector<llvm::Type*> result;
  for (auto x : elem_types) {
    result.push_back(types_[x.GetID()]);
  }
  return result;
}

void LLVMTypeManager::TranslateFunctionType(Type result,
                                            absl::Span<const Type> arg_types) {
  types_.push_back(llvm::FunctionType::get(types_[result.GetID()],
                                           GetTypeArray(arg_types), false));
}

void LLVMTypeManager::TranslateStructType(absl::Span<const Type> elem_types) {
  types_.push_back(
      llvm::StructType::create(*context_, GetTypeArray(elem_types)));
}

const std::vector<llvm::Type*>& LLVMTypeManager::GetTypes() { return types_; }

std::string GetGlobalName(int id) { return "_global" + std::to_string(id++); }

llvm::Function* DeclareFunction(const Function& func, llvm::Module* mod,
                                const std::vector<llvm::Type*>& types) {
  std::string fn_name(func.Name());
  auto type = llvm::dyn_cast<llvm::FunctionType>(types[func.Type().GetID()]);
  auto linkage = llvm::GlobalValue::LinkageTypes::ExternalLinkage;
  return llvm::Function::Create(type, linkage, fn_name.c_str(), *mod);
}

double LLVMBackend::compilation_time_ = 0;

LLVMBackend::LLVMBackend(const khir::Program& program)
    : program_(program), functions_(program_.Functions().size(), nullptr) {
#ifdef COMP_TIME
  auto t1 = std::chrono::high_resolution_clock::now();
#endif
  auto target_triple = llvm::sys::getDefaultTargetTriple();
  std::string error;
  auto target = llvm::TargetRegistry::lookupTarget(target_triple, error);
  if (!target) {
    throw std::runtime_error("Target not found: " + error);
  }

  auto cpu = "x86-64";
  auto features = "+avx2";

  llvm::TargetOptions opt;
  auto reloc_model =
      llvm::Optional<llvm::Reloc::Model>(llvm::Reloc::Model::PIC_);
  auto target_machine = target->createTargetMachine(target_triple, cpu,
                                                    features, opt, reloc_model);

  jit_ = cantFail(
      llvm::orc::LLJITBuilder()
          .setDataLayout(target_machine->createDataLayout())
          .setNumCompileThreads(0)
          .setObjectLinkingLayerCreator([&](llvm::orc::ExecutionSession& es,
                                            const llvm::Triple& tt) {
            auto ll =
                std::make_unique<llvm::orc::RTDyldObjectLinkingLayer>(es, []() {
                  return std::make_unique<llvm::SectionMemoryManager>();
                });
            ll->setAutoClaimResponsibilityForObjectSymbols(true);
#ifdef PROFILE_ENABLED
            auto perf = createPerfJITEventListener();
            ll->registerJITEventListener(*perf);
#endif
            return ll;
          })
          .create());

  llvm::orc::SymbolMap symbol_map;
  for (const auto& func : program_.Functions()) {
    if (func.External()) {
      auto mangled = jit_->mangleAndIntern(func.Name());
      auto flags =
          llvm::JITSymbolFlags::Callable | llvm::JITSymbolFlags::Exported;
      auto symbol = llvm::JITEvaluatedSymbol(
          llvm::pointerToJITTargetAddress(func.Addr()), flags);
      symbol_map.try_emplace(mangled, symbol);
      continue;
    }
  }
  cantFail(
      jit_->getMainJITDylib().define(llvm::orc::absoluteSymbols(symbol_map)));

  {  // output all global variables
    auto context = std::make_unique<llvm::LLVMContext>();
    auto mod = std::make_unique<llvm::Module>("query", *context);
    auto builder = std::make_unique<llvm::IRBuilder<>>(*context);

    // Compute types array
    LLVMTypeManager type_manager(context.get(), builder.get());
    program_.TypeManager().Translate(type_manager);
    const auto& types = type_manager.GetTypes();

    std::vector<llvm::Constant*> constant_values(
        program_.ConstantInstrs().size(), nullptr);

    const auto& funcs = program_.Functions();
    for (int i = 0; i < funcs.size(); i++) {
      if (funcs[i].External()) {
        functions_[i] = DeclareFunction(funcs[i], mod.get(), types);
      }
    }

    const auto& globals = program_.Globals();
    for (int id = 0; id < globals.size(); id++) {
      const auto& global = globals[id];
      auto type = types[global.Type().GetID()];
      auto init = GetConstant(global.InitialValue(), mod.get(), context.get(),
                              builder.get(), types, constant_values);
      new llvm::GlobalVariable(*mod, type, false,
                               llvm::GlobalValue::LinkageTypes::ExternalLinkage,
                               init, GetGlobalName(id));
    }

    std::vector<std::string_view> to_add;
    while (!to_translate_.empty()) {
      auto curr = to_translate_.front();
      to_translate_.pop();

      to_add.push_back(funcs[curr].Name());

      TranslateFunction(funcs[curr], mod.get(), context.get(), builder.get(),
                        types, constant_values);
    }

    CompileAndLink(std::move(mod), std::move(context), to_add);
  }
#ifdef COMP_TIME
  auto t2 = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> fp_ms = t2 - t1;
  compilation_time_ += fp_ms.count();
#endif
}

llvm::Constant* LLVMBackend::GetConstant(
    Value v, llvm::Module* mod, llvm::LLVMContext* context,
    llvm::IRBuilder<>* builder, const std::vector<llvm::Type*>& types,
    std::vector<llvm::Constant*>& constant_values) {
  if (!v.IsConstantGlobal()) {
    throw std::runtime_error("Invalid constant/global.");
  }

  auto instr = program_.ConstantInstrs()[v.GetIdx()];
  auto& dest = constant_values[v.GetIdx()];
  if (dest != nullptr) {
    return dest;
  }

  auto opcode = ConstantOpcodeFrom(GenericInstructionReader(instr).Opcode());
  switch (opcode) {
    case ConstantOpcode::I1_CONST: {
      dest = builder->getInt1(Type1InstructionReader(instr).Constant() == 1);
      break;
    }

    case ConstantOpcode::I8_CONST: {
      dest = builder->getInt8(Type1InstructionReader(instr).Constant());
      break;
    }

    case ConstantOpcode::I16_CONST: {
      dest = builder->getInt16(Type1InstructionReader(instr).Constant());
      break;
    }

    case ConstantOpcode::I32_CONST: {
      dest = builder->getInt32(Type1InstructionReader(instr).Constant());
      break;
    }

    case ConstantOpcode::I64_CONST: {
      dest = builder->getInt64(
          program_.I64Constants()[Type1InstructionReader(instr).Constant()]);
      break;
    }

    case ConstantOpcode::F64_CONST: {
      dest = llvm::ConstantFP::get(
          builder->getDoubleTy(),
          program_.F64Constants()[Type1InstructionReader(instr).Constant()]);
      break;
    }

    case ConstantOpcode::PTR_CONST: {
      auto i64_v = builder->getInt64(reinterpret_cast<uint64_t>(
          program_.PtrConstants()[Type1InstructionReader(instr).Constant()]));
      dest = llvm::ConstantExpr::getIntToPtr(i64_v, builder->getInt8PtrTy());
      break;
    }

    case ConstantOpcode::PTR_CAST: {
      Type3InstructionReader reader(instr);
      Value ptr(reader.Arg());
      auto t = types[reader.TypeID()];
      auto v = GetConstant(ptr, mod, context, builder, types, constant_values);
      dest = llvm::ConstantExpr::getBitCast(v, t);
      break;
    }

    case ConstantOpcode::GLOBAL_CHAR_ARRAY_CONST: {
      dest = builder->CreateGlobalStringPtr(
          program_
              .CharArrayConstants()[Type1InstructionReader(instr).Constant()],
          "", 0, mod);
      break;
    }

    case ConstantOpcode::STRUCT_CONST: {
      const auto& x =
          program_.StructConstants()[Type1InstructionReader(instr).Constant()];
      std::vector<llvm::Constant*> init;
      for (auto field_init : x.Fields()) {
        init.push_back(GetConstant(field_init, mod, context, builder, types,
                                   constant_values));
      }
      auto* st = llvm::dyn_cast<llvm::StructType>(types[x.Type().GetID()]);
      dest = llvm::ConstantStruct::get(st, init);
      break;
    }

    case ConstantOpcode::ARRAY_CONST: {
      const auto& x =
          program_.ArrayConstants()[Type1InstructionReader(instr).Constant()];

      std::vector<llvm::Constant*> init;
      for (auto element_init : x.Elements()) {
        init.push_back(GetConstant(element_init, mod, context, builder, types,
                                   constant_values));
      }
      auto* at = llvm::dyn_cast<llvm::ArrayType>(types[x.Type().GetID()]);
      dest = llvm::ConstantArray::get(at, init);
      break;
    }

    case ConstantOpcode::NULLPTR: {
      auto t = types[Type3InstructionReader(instr).TypeID()];
      dest =
          llvm::ConstantPointerNull::get(llvm::dyn_cast<llvm::PointerType>(t));
      break;
    }

    case ConstantOpcode::GLOBAL_REF: {
      auto id = Type1InstructionReader(instr).Constant();
      const auto& global = program_.Globals()[id];
      auto name = GetGlobalName(id);

      {
        auto exists = mod->getGlobalVariable(name);
        if (exists) {
          dest = exists;
          break;
        }
      }

      auto type = types[global.Type().GetID()];
      dest = new llvm::GlobalVariable(
          *mod, type, false, llvm::GlobalValue::LinkageTypes::ExternalLinkage,
          nullptr, GetGlobalName(id), nullptr,
          llvm::GlobalValue::NotThreadLocal, llvm::None, true);
      break;
    }

    case ConstantOpcode::FUNC_PTR: {
      auto id = Type3InstructionReader(instr).Arg();
      if (functions_[id] == nullptr) {
        functions_[id] = DeclareFunction(program_.Functions()[id], mod, types);
        to_translate_.push(id);
      }
      auto name = program_.Functions()[id].Name();
      dest = mod->getFunction(name);
      break;
    }

    case ConstantOpcode::I32_VEC4_CONST_4: {
      auto idx = Type1InstructionReader(instr).Constant();
      std::vector<llvm::Constant*> constants;
      for (int x : program_.I32Vec4Constants()[idx]) {
        constants.push_back(builder->getInt32(x));
      }
      dest = llvm::ConstantVector::get(constants);
      break;
    }

    case ConstantOpcode::I32_VEC8_CONST_1: {
      auto x = Type1InstructionReader(instr).Constant();
      std::vector<llvm::Constant*> constants;
      for (int i = 0; i < 8; i++) {
        constants.push_back(builder->getInt32(x));
      }
      dest = llvm::ConstantVector::get(constants);
      break;
    }

    case ConstantOpcode::I32_VEC8_CONST_8: {
      auto idx = Type1InstructionReader(instr).Constant();
      std::vector<llvm::Constant*> constants;
      for (int x : program_.I32Vec8Constants()[idx]) {
        constants.push_back(builder->getInt32(x));
      }
      dest = llvm::ConstantVector::get(constants);
      break;
    }
  }

  return dest;
}

void LLVMBackend::TranslateFunction(
    const Function& func, llvm::Module* mod, llvm::LLVMContext* context,
    llvm::IRBuilder<>* builder, const std::vector<llvm::Type*>& types,
    std::vector<llvm::Constant*>& constant_values) {
  llvm::Function* function = mod->getFunction(func.Name());
  std::vector<llvm::Value*> args;
  for (auto& a : function->args()) {
    args.push_back(&a);
  }

  const auto& instructions = func.Instrs();
  const auto& basic_blocks = func.BasicBlocks();

  absl::flat_hash_map<uint32_t,
                      std::vector<std::pair<llvm::Value*, llvm::BasicBlock*>>>
      phi_member_list;
  std::vector<llvm::Value*> values(instructions.size(), nullptr);
  std::vector<llvm::Value*> call_args;

  std::vector<llvm::BasicBlock*> basic_blocks_impl;
  for (int i = 0; i < basic_blocks.size(); i++) {
    basic_blocks_impl.push_back(
        llvm::BasicBlock::Create(*context, "", function));
  }

  for (int i = 0; i < basic_blocks.size(); i++) {
    builder->SetInsertPoint(basic_blocks_impl[i]);
    for (const auto& [b_start, b_end] : basic_blocks[i].Segments()) {
      for (int instr_idx = b_start; instr_idx <= b_end; instr_idx++) {
        TranslateInstr(instr_idx, instructions, mod, context, builder, types,
                       args, basic_blocks_impl, values, call_args,
                       constant_values, phi_member_list);
      }
    }
  }
}

double LLVMBackend::CompilationTime() { return compilation_time_; }

void LLVMBackend::ResetCompilationTime() { compilation_time_ = 0; }

void LLVMBackend::Translate(std::string_view name) {
  auto context = std::make_unique<llvm::LLVMContext>();
  auto mod = std::make_unique<llvm::Module>("query", *context);
  auto builder = std::make_unique<llvm::IRBuilder<>>(*context);

  // Compute types array
  LLVMTypeManager type_manager(context.get(), builder.get());
  program_.TypeManager().Translate(type_manager);
  const auto& types = type_manager.GetTypes();

  // Translate all previously created functions
  const auto& funcs = program_.Functions();
  int id = -1;
  functions_ = std::vector<llvm::Function*>(funcs.size(), nullptr);
  for (int i = 0; i < funcs.size(); i++) {
    if (funcs[i].External() || compiled_fn_.contains(funcs[i].Name())) {
      functions_[i] = DeclareFunction(funcs[i], mod.get(), types);
    }

    if (funcs[i].Name() == name) {
      id = i;
    }
  }

  std::vector<llvm::Constant*> constant_values(program_.ConstantInstrs().size(),
                                               nullptr);

  functions_[id] = DeclareFunction(funcs[id], mod.get(), types);
  to_translate_.push(id);

  // Translate all func bodies
  std::vector<std::string_view> to_add;
  while (!to_translate_.empty()) {
    auto curr = to_translate_.front();
    to_translate_.pop();
    to_add.push_back(funcs[curr].Name());
    TranslateFunction(funcs[curr], mod.get(), context.get(), builder.get(),
                      types, constant_values);
  }

  CompileAndLink(std::move(mod), std::move(context), to_add);
}

using LLVMCmp = llvm::CmpInst::Predicate;

LLVMCmp GetLLVMCompType(Opcode opcode) {
  switch (opcode) {
    case Opcode::I1_CMP_EQ:
    case Opcode::I8_CMP_EQ:
    case Opcode::I16_CMP_EQ:
    case Opcode::I32_CMP_EQ:
    case Opcode::I64_CMP_EQ:
    case Opcode::I32_VEC8_CMP_EQ:
      return LLVMCmp::ICMP_EQ;

    case Opcode::I1_CMP_NE:
    case Opcode::I8_CMP_NE:
    case Opcode::I16_CMP_NE:
    case Opcode::I32_CMP_NE:
    case Opcode::I64_CMP_NE:
    case Opcode::I32_VEC8_CMP_NE:
      return LLVMCmp::ICMP_NE;

    case Opcode::I8_CMP_LT:
    case Opcode::I16_CMP_LT:
    case Opcode::I32_CMP_LT:
    case Opcode::I64_CMP_LT:
    case Opcode::I32_VEC8_CMP_LT:
      return LLVMCmp::ICMP_SLT;

    case Opcode::I8_CMP_LE:
    case Opcode::I16_CMP_LE:
    case Opcode::I32_CMP_LE:
    case Opcode::I64_CMP_LE:
    case Opcode::I32_VEC8_CMP_LE:
      return LLVMCmp::ICMP_SLE;

    case Opcode::I8_CMP_GT:
    case Opcode::I16_CMP_GT:
    case Opcode::I32_CMP_GT:
    case Opcode::I64_CMP_GT:
    case Opcode::I32_VEC8_CMP_GT:
      return LLVMCmp::ICMP_SGT;

    case Opcode::I8_CMP_GE:
    case Opcode::I16_CMP_GE:
    case Opcode::I32_CMP_GE:
    case Opcode::I64_CMP_GE:
    case Opcode::I32_VEC8_CMP_GE:
      return LLVMCmp::ICMP_SGE;

    case Opcode::F64_CMP_EQ:
      return LLVMCmp::FCMP_OEQ;

    case Opcode::F64_CMP_NE:
      return LLVMCmp::FCMP_ONE;

    case Opcode::F64_CMP_LT:
      return LLVMCmp::FCMP_OLT;

    case Opcode::F64_CMP_LE:
      return LLVMCmp::FCMP_OLE;

    case Opcode::F64_CMP_GT:
      return LLVMCmp::FCMP_OGT;

    case Opcode::F64_CMP_GE:
      return LLVMCmp::FCMP_OGE;

    default:
      throw std::runtime_error("Not a comparison opcode.");
  }
}

llvm::Value* LLVMBackend::GetValue(
    khir::Value v, std::vector<llvm::Constant*>& constant_values,
    const std::vector<llvm::Value*>& values, llvm::Module* mod,
    llvm::LLVMContext* context, llvm::IRBuilder<>* builder,
    const std::vector<llvm::Type*>& types) {
  if (v.IsConstantGlobal()) {
    return GetConstant(v, mod, context, builder, types, constant_values);
  } else {
    return values[v.GetIdx()];
  }
}

void LLVMBackend::TranslateInstr(
    int instr_idx, const std::vector<uint64_t>& instructions, llvm::Module* mod,
    llvm::LLVMContext* context, llvm::IRBuilder<>* builder,
    const std::vector<llvm::Type*>& types,
    const std::vector<llvm::Value*>& func_args,
    const std::vector<llvm::BasicBlock*>& basic_blocks,
    std::vector<llvm::Value*>& values, std::vector<llvm::Value*>& call_args,
    std::vector<llvm::Constant*>& constant_values,
    absl::flat_hash_map<
        uint32_t, std::vector<std::pair<llvm::Value*, llvm::BasicBlock*>>>&
        phi_member_list) {
  auto instr = instructions[instr_idx];
  auto opcode = OpcodeFrom(GenericInstructionReader(instr).Opcode());

  switch (opcode) {
    case Opcode::I1_VEC8_AND:
    case Opcode::I1_AND:
    case Opcode::I64_AND: {
      Type2InstructionReader reader(instr);
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                         context, builder, types);
      auto v1 = GetValue(Value(reader.Arg1()), constant_values, values, mod,
                         context, builder, types);
      values[instr_idx] = builder->CreateAnd(v0, v1);
      return;
    }

    case Opcode::I1_VEC8_OR:
    case Opcode::I1_OR:
    case Opcode::I64_OR: {
      Type2InstructionReader reader(instr);
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                         context, builder, types);
      auto v1 = GetValue(Value(reader.Arg1()), constant_values, values, mod,
                         context, builder, types);
      values[instr_idx] = builder->CreateOr(v0, v1);
      return;
    }

    case Opcode::I64_XOR: {
      Type2InstructionReader reader(instr);
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                         context, builder, types);
      auto v1 = GetValue(Value(reader.Arg1()), constant_values, values, mod,
                         context, builder, types);
      values[instr_idx] = builder->CreateXor(v0, v1);
      return;
    }

    case Opcode::I1_CMP_EQ:
    case Opcode::I1_CMP_NE:
    case Opcode::I8_CMP_EQ:
    case Opcode::I8_CMP_NE:
    case Opcode::I8_CMP_LT:
    case Opcode::I8_CMP_LE:
    case Opcode::I8_CMP_GT:
    case Opcode::I8_CMP_GE:
    case Opcode::I16_CMP_EQ:
    case Opcode::I16_CMP_NE:
    case Opcode::I16_CMP_LT:
    case Opcode::I16_CMP_LE:
    case Opcode::I16_CMP_GT:
    case Opcode::I16_CMP_GE:
    case Opcode::I32_CMP_EQ:
    case Opcode::I32_CMP_NE:
    case Opcode::I32_CMP_LT:
    case Opcode::I32_CMP_LE:
    case Opcode::I32_CMP_GT:
    case Opcode::I32_CMP_GE:
    case Opcode::I32_VEC8_CMP_EQ:
    case Opcode::I32_VEC8_CMP_NE:
    case Opcode::I32_VEC8_CMP_LT:
    case Opcode::I32_VEC8_CMP_LE:
    case Opcode::I32_VEC8_CMP_GT:
    case Opcode::I32_VEC8_CMP_GE:
    case Opcode::I64_CMP_EQ:
    case Opcode::I64_CMP_NE:
    case Opcode::I64_CMP_LT:
    case Opcode::I64_CMP_LE:
    case Opcode::I64_CMP_GT:
    case Opcode::I64_CMP_GE:
    case Opcode::F64_CMP_EQ:
    case Opcode::F64_CMP_NE:
    case Opcode::F64_CMP_LT:
    case Opcode::F64_CMP_LE:
    case Opcode::F64_CMP_GT:
    case Opcode::F64_CMP_GE: {
      Type2InstructionReader reader(instr);
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                         context, builder, types);
      auto v1 = GetValue(Value(reader.Arg1()), constant_values, values, mod,
                         context, builder, types);
      auto comp_type = GetLLVMCompType(opcode);
      values[instr_idx] = builder->CreateCmp(comp_type, v0, v1);
      return;
    }

    case Opcode::I32_CMP_EQ_ANY_CONST_VEC4: {
      Type2InstructionReader reader(instr);
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                         context, builder, types);
      auto v1 = GetValue(Value(reader.Arg1()), constant_values, values, mod,
                         context, builder, types);

      llvm::Value* vec = llvm::ConstantVector::get(
          std::vector<llvm::Constant*>(4, builder->getInt32(0)));
      for (int i = 0; i < 4; i++) {
        vec = builder->CreateInsertElement(vec, v0, i);
      }

      auto comp =
          builder->CreateCmp(GetLLVMCompType(Opcode::I32_CMP_EQ), vec, v1);
      values[instr_idx] = builder->CreateOrReduce(comp);
      return;
    }

    case Opcode::I32_CMP_EQ_ANY_CONST_VEC8: {
      Type2InstructionReader reader(instr);
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                         context, builder, types);
      auto v1 = GetValue(Value(reader.Arg1()), constant_values, values, mod,
                         context, builder, types);

      llvm::Value* vec = llvm::ConstantVector::get(
          std::vector<llvm::Constant*>(8, builder->getInt32(0)));
      for (int i = 0; i < 8; i++) {
        vec = builder->CreateInsertElement(vec, v0, i);
      }

      auto comp =
          builder->CreateCmp(GetLLVMCompType(Opcode::I32_CMP_EQ), vec, v1);
      values[instr_idx] = builder->CreateOrReduce(comp);
      return;
    }

    case Opcode::PTR_CMP_NULLPTR: {
      Type2InstructionReader reader(instr);
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                         context, builder, types);
      auto v1 = llvm::ConstantPointerNull::get(
          llvm::dyn_cast<llvm::PointerType>(v0->getType()));
      values[instr_idx] = builder->CreateCmp(LLVMCmp::ICMP_EQ, v0, v1);
      return;
    }

    case Opcode::I8_ADD:
    case Opcode::I16_ADD:
    case Opcode::I32_ADD:
    case Opcode::I64_ADD:
    case Opcode::I32_VEC8_ADD: {
      Type2InstructionReader reader(instr);
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                         context, builder, types);
      auto v1 = GetValue(Value(reader.Arg1()), constant_values, values, mod,
                         context, builder, types);
      values[instr_idx] = builder->CreateAdd(v0, v1);
      return;
    }

    case Opcode::I64_LSHIFT: {
      Type2InstructionReader reader(instr);
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                         context, builder, types);
      auto v1 = GetValue(Value(reader.Arg1()), constant_values, values, mod,
                         context, builder, types);
      values[instr_idx] = builder->CreateShl(v0, v1);
      return;
    }

    case Opcode::I64_RSHIFT: {
      Type2InstructionReader reader(instr);
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                         context, builder, types);
      auto v1 = GetValue(Value(reader.Arg1()), constant_values, values, mod,
                         context, builder, types);
      values[instr_idx] = builder->CreateLShr(v0, v1);
      return;
    }

    case Opcode::I8_MUL:
    case Opcode::I16_MUL:
    case Opcode::I32_MUL:
    case Opcode::I64_MUL: {
      Type2InstructionReader reader(instr);
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                         context, builder, types);
      auto v1 = GetValue(Value(reader.Arg1()), constant_values, values, mod,
                         context, builder, types);
      values[instr_idx] = builder->CreateMul(v0, v1);
      return;
    }

    case Opcode::I8_SUB:
    case Opcode::I16_SUB:
    case Opcode::I32_SUB:
    case Opcode::I64_SUB: {
      Type2InstructionReader reader(instr);
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                         context, builder, types);
      auto v1 = GetValue(Value(reader.Arg1()), constant_values, values, mod,
                         context, builder, types);
      values[instr_idx] = builder->CreateSub(v0, v1);
      return;
    }

    case Opcode::I1_ZEXT_I8: {
      Type2InstructionReader reader(instr);
      auto v = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                        context, builder, types);
      values[instr_idx] = builder->CreateZExt(v, builder->getInt8Ty());
      return;
    }

    case Opcode::I1_ZEXT_I64:
    case Opcode::I8_ZEXT_I64:
    case Opcode::I16_ZEXT_I64:
    case Opcode::I32_ZEXT_I64: {
      Type2InstructionReader reader(instr);
      auto v = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                        context, builder, types);
      values[instr_idx] = builder->CreateZExt(v, builder->getInt64Ty());
      return;
    }

    case Opcode::I32_VEC8_PERMUTE: {
      Type2InstructionReader reader(instr);
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                         context, builder, types);
      auto v1 = GetValue(Value(reader.Arg1()), constant_values, values, mod,
                         context, builder, types);
      auto perm =
          llvm::Intrinsic::getDeclaration(mod, llvm::Intrinsic::x86_avx2_permd);
      values[instr_idx] = builder->CreateCall(perm, {v0, v1});
      return;
    }

    case Opcode::I1_VEC8_NOT: {
      Type2InstructionReader reader(instr);
      auto v = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                        context, builder, types);
      values[instr_idx] = builder->CreateXor(
          v, llvm::ConstantVector::get(
                 std::vector<llvm::Constant*>(8, builder->getInt1(true))));
      return;
    }

    case Opcode::MASK_TO_PERMUTE: {
      Type2InstructionReader reader(instr);
      auto v = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                        context, builder, types);
      values[instr_idx] = builder->CreateIntToPtr(
          builder->CreateAdd(builder->CreateShl(v, builder->getInt64(5)),
                             builder->getInt64(reinterpret_cast<uint64_t>(
                                 util::PermutationTable::Get().Addr()))),
          llvm::PointerType::get(
              llvm::FixedVectorType::get(builder->getInt32Ty(), 8), 0));
      return;
    }

    case Opcode::I64_POPCOUNT: {
      Type2InstructionReader reader(instr);
      auto v = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                        context, builder, types);
      auto perm = llvm::Intrinsic::getDeclaration(mod, llvm::Intrinsic::ctpop,
                                                  {builder->getInt64Ty()});
      values[instr_idx] = builder->CreateCall(perm, {v});
      return;
    }

    case Opcode::I1_VEC8_MASK_EXTRACT: {
      Type2InstructionReader reader(instr);
      auto v = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                        context, builder, types);
      values[instr_idx] =
          builder->CreateZExt(builder->CreateBitCast(v, builder->getInt8Ty()),
                              builder->getInt64Ty());
      return;
    }

    case Opcode::I1_LNOT: {
      Type2InstructionReader reader(instr);
      auto v = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                        context, builder, types);
      values[instr_idx] = builder->CreateXor(v, builder->getInt1(true));
      return;
    }

    case Opcode::F64_ADD: {
      Type2InstructionReader reader(instr);
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                         context, builder, types);
      auto v1 = GetValue(Value(reader.Arg1()), constant_values, values, mod,
                         context, builder, types);
      values[instr_idx] = builder->CreateFAdd(v0, v1);
      return;
    }

    case Opcode::F64_MUL: {
      Type2InstructionReader reader(instr);
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                         context, builder, types);
      auto v1 = GetValue(Value(reader.Arg1()), constant_values, values, mod,
                         context, builder, types);
      values[instr_idx] = builder->CreateFMul(v0, v1);
      return;
    }

    case Opcode::F64_SUB: {
      Type2InstructionReader reader(instr);
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                         context, builder, types);
      auto v1 = GetValue(Value(reader.Arg1()), constant_values, values, mod,
                         context, builder, types);
      values[instr_idx] = builder->CreateFSub(v0, v1);
      return;
    }

    case Opcode::F64_DIV: {
      Type2InstructionReader reader(instr);
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                         context, builder, types);
      auto v1 = GetValue(Value(reader.Arg1()), constant_values, values, mod,
                         context, builder, types);
      values[instr_idx] = builder->CreateFDiv(v0, v1);
      return;
    }

    case Opcode::F64_CONV_I64: {
      Type2InstructionReader reader(instr);
      auto v = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                        context, builder, types);
      values[instr_idx] = builder->CreateFPToSI(v, builder->getInt64Ty());
      return;
    }

    case Opcode::I64_TRUNC_I16: {
      Type2InstructionReader reader(instr);
      auto v = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                        context, builder, types);
      values[instr_idx] = builder->CreateTrunc(v, builder->getInt16Ty());
      return;
    }

    case Opcode::I64_TRUNC_I32: {
      Type2InstructionReader reader(instr);
      auto v = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                        context, builder, types);
      values[instr_idx] = builder->CreateTrunc(v, builder->getInt32Ty());
      return;
    }

    case Opcode::I32_CONV_I32_VEC8: {
      Type2InstructionReader reader(instr);
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                         context, builder, types);

      llvm::Value* vec = llvm::ConstantVector::get(
          std::vector<llvm::Constant*>(8, builder->getInt32(0)));
      for (int i = 0; i < 8; i++) {
        vec = builder->CreateInsertElement(vec, v0, i);
      }

      values[instr_idx] = vec;
      return;
    }

    case Opcode::I8_CONV_F64:
    case Opcode::I16_CONV_F64:
    case Opcode::I32_CONV_F64:
    case Opcode::I64_CONV_F64: {
      Type2InstructionReader reader(instr);
      auto v = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                        context, builder, types);
      values[instr_idx] = builder->CreateSIToFP(v, builder->getDoubleTy());
      return;
    }

    case Opcode::BR: {
      Type5InstructionReader reader(instr);
      auto bb = basic_blocks[reader.Marg0()];
      values[instr_idx] = builder->CreateBr(bb);
      return;
    }

    case Opcode::CONDBR: {
      Type5InstructionReader reader(instr);
      auto v = GetValue(Value(reader.Arg()), constant_values, values, mod,
                        context, builder, types);
      auto bb0 = basic_blocks[reader.Marg0()];
      auto bb1 = basic_blocks[reader.Marg1()];
      values[instr_idx] = builder->CreateCondBr(v, bb0, bb1);
      return;
    }

    case Opcode::RETURN: {
      values[instr_idx] = builder->CreateRetVoid();
      return;
    }

    case Opcode::RETURN_VALUE: {
      Type3InstructionReader reader(instr);
      auto v = GetValue(Value(reader.Arg()), constant_values, values, mod,
                        context, builder, types);
      values[instr_idx] = builder->CreateRet(v);
      return;
    }

    case Opcode::I8_STORE:
    case Opcode::I16_STORE:
    case Opcode::I32_STORE:
    case Opcode::I64_STORE:
    case Opcode::F64_STORE:
    case Opcode::PTR_STORE: {
      Type2InstructionReader reader(instr);
      auto ptr = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                          context, builder, types);
      auto val = GetValue(Value(reader.Arg1()), constant_values, values, mod,
                          context, builder, types);
      values[instr_idx] = builder->CreateStore(val, ptr);
      return;
    }

    case Opcode::I32_VEC8_MASK_STORE_INFO: {
      return;
    }

    case Opcode::I32_VEC8_MASK_STORE: {
      Type2InstructionReader reader(instr);
      Type2InstructionReader reader_info(instructions[instr_idx - 1]);

      auto ptr = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                          context, builder, types);
      auto val = GetValue(Value(reader.Arg1()), constant_values, values, mod,
                          context, builder, types);
      auto popcount = GetValue(Value(reader_info.Arg0()), constant_values,
                               values, mod, context, builder, types);

      std::vector<llvm::Constant*> init;
      auto i32_arr8_ty = llvm::ArrayType::get(builder->getInt32Ty(), 8);
      for (int i = 0; i < 9; i++) {
        std::vector<llvm::Constant*> values;
        for (int j = 0; j < i; j++) {
          values.push_back(builder->getInt32(-1));
        }
        for (int j = i; j < 8; j++) {
          values.push_back(builder->getInt32(0));
        }
        init.push_back(llvm::ConstantArray::get(i32_arr8_ty, values));
      }
      auto* at = llvm::ArrayType::get(i32_arr8_ty, 9);
      auto constant_arr = llvm::ConstantArray::get(at, init);
      auto global = new llvm::GlobalVariable(
          *mod, at, true, llvm::GlobalValue::LinkageTypes::InternalLinkage,
          constant_arr);
      global->setAlignment(llvm::MaybeAlign(32));
      auto mask_ptr = builder->CreateGEP(
          at, global, {builder->getInt32(0), popcount, builder->getInt32(0)});

      auto i32_vec8_ty = llvm::FixedVectorType::get(builder->getInt32Ty(), 8);
      auto mask = builder->CreateLoad(
          i32_vec8_ty, builder->CreatePointerCast(
                           mask_ptr, llvm::PointerType::get(i32_vec8_ty, 0)));

      auto perm = llvm::Intrinsic::getDeclaration(
          mod, llvm::Intrinsic::x86_avx2_maskstore_d_256);
      builder->CreateCall(
          perm, {builder->CreatePointerCast(ptr, builder->getInt8PtrTy()), mask,
                 val});
    }

    case Opcode::I1_LOAD:
    case Opcode::I8_LOAD:
    case Opcode::I16_LOAD:
    case Opcode::I32_LOAD:
    case Opcode::I64_LOAD:
    case Opcode::F64_LOAD:
    case Opcode::I32_VEC8_LOAD: {
      Type2InstructionReader reader(instr);
      auto v = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                        context, builder, types);
      values[instr_idx] =
          builder->CreateLoad(v->getType()->getPointerElementType(), v);
      return;
    }

    case Opcode::PTR_LOAD: {
      Type3InstructionReader reader(instr);
      auto v = GetValue(Value(reader.Arg()), constant_values, values, mod,
                        context, builder, types);
      values[instr_idx] =
          builder->CreateLoad(v->getType()->getPointerElementType(), v);
      return;
    }

    case Opcode::PTR_CAST: {
      Type3InstructionReader reader(instr);
      auto t = types[reader.TypeID()];
      auto v = GetValue(Value(reader.Arg()), constant_values, values, mod,
                        context, builder, types);
      values[instr_idx] = builder->CreatePointerCast(v, t);
      return;
    }

    case Opcode::GEP_STATIC_OFFSET: {
      return;
    }

    case Opcode::GEP_STATIC: {
      Type3InstructionReader reader(instr);
      auto t = types[reader.TypeID()];
      llvm::Value* base;

      {
        Type2InstructionReader reader(instructions[instr_idx - 1]);
        auto ptr = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                            context, builder, types);
        auto offset = GetValue(Value(reader.Arg1()), constant_values, values,
                               mod, context, builder, types);

        auto ptr_as_int = builder->CreatePtrToInt(ptr, builder->getInt64Ty());
        auto ptr_plus_offset = builder->CreateAdd(
            ptr_as_int, builder->CreateZExt(offset, builder->getInt64Ty()));

        base =
            builder->CreateIntToPtr(ptr_plus_offset, builder->getInt8PtrTy());
      }

      values[instr_idx] = builder->CreatePointerCast(base, t);
      return;
    }

    case Opcode::GEP_DYNAMIC_OFFSET: {
      return;
    }

    case Opcode::GEP_DYNAMIC: {
      Type3InstructionReader reader(instr);
      auto t = types[reader.TypeID()];
      auto type_size = reader.Sarg();
      auto ptr = GetValue(Value(reader.Arg()), constant_values, values, mod,
                          context, builder, types);
      llvm::Value* base;

      {
        Type2InstructionReader reader(instructions[instr_idx - 1]);
        auto idx = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                            context, builder, types);
        auto offset = GetValue(Value(reader.Arg1()), constant_values, values,
                               mod, context, builder, types);

        auto ptr_as_int = builder->CreatePtrToInt(ptr, builder->getInt64Ty());
        auto ptr_offset1 = builder->CreateAdd(
            ptr_as_int,
            builder->CreateMul(builder->CreateZExt(idx, builder->getInt64Ty()),
                               builder->getInt64(type_size)));
        auto ptr_offset1_offset2 = builder->CreateAdd(
            ptr_offset1, builder->CreateZExt(offset, builder->getInt64Ty()));
        base = builder->CreateIntToPtr(ptr_offset1_offset2,
                                       builder->getInt8PtrTy());
      }

      values[instr_idx] = builder->CreatePointerCast(base, t);
      return;
    }

    case Opcode::CALL_ARG: {
      Type3InstructionReader reader(instr);
      auto v = GetValue(Value(reader.Arg()), constant_values, values, mod,
                        context, builder, types);
      call_args.push_back(v);
      return;
    }

    case Opcode::CALL: {
      auto id = Type3InstructionReader(instr).Arg();
      if (functions_[id] == nullptr) {
        functions_[id] = DeclareFunction(program_.Functions()[id], mod, types);
        to_translate_.push(id);
      }
      auto func = mod->getFunction(program_.Functions()[id].Name());
      values[instr_idx] = builder->CreateCall(func, call_args);
      call_args.clear();
      return;
    }

    case Opcode::CALL_INDIRECT: {
      Type3InstructionReader reader(instr);
      auto func = GetValue(Value(reader.Arg()), constant_values, values, mod,
                           context, builder, types);
      auto func_type = func->getType()->getPointerElementType();
      values[instr_idx] = llvm::CallInst::Create(
          llvm::dyn_cast<llvm::FunctionType>(func_type), func, call_args, "",
          builder->GetInsertBlock());
      call_args.clear();
      return;
    }

    case Opcode::FUNC_ARG: {
      Type3InstructionReader reader(instr);
      auto arg_idx = reader.Sarg();
      values[instr_idx] = func_args[arg_idx];
      return;
    }

    case Opcode::PHI_MEMBER: {
      Type2InstructionReader reader(instr);
      auto phi = GetValue(Value(reader.Arg0()), constant_values, values, mod,
                          context, builder, types);
      auto phi_member = GetValue(Value(reader.Arg1()), constant_values, values,
                                 mod, context, builder, types);

      if (phi == nullptr) {
        phi_member_list[reader.Arg0()].emplace_back(phi_member,
                                                    builder->GetInsertBlock());
      } else {
        llvm::dyn_cast<llvm::PHINode>(phi)->addIncoming(
            phi_member, builder->GetInsertBlock());
      }
      return;
    }

    case Opcode::PHI: {
      Type3InstructionReader reader(instr);
      auto t = types[reader.TypeID()];

      auto phi = builder->CreatePHI(t, 2);
      values[instr_idx] = phi;

      if (phi_member_list.contains(instr_idx)) {
        for (const auto& [phi_member, basic_block] :
             phi_member_list[instr_idx]) {
          phi->addIncoming(phi_member, basic_block);
        }

        phi_member_list.erase(instr_idx);
      }

      return;
    }
  }
}

void LLVMBackend::CompileAndLink(std::unique_ptr<llvm::Module> mod,
                                 std::unique_ptr<llvm::LLVMContext> context,
                                 const std::vector<std::string_view>& to_add) {
  llvm::verifyModule(*mod, &llvm::errs());

  llvm::legacy::PassManager pass;
  pass.add(llvm::createInstructionCombiningPass());
  pass.add(llvm::createReassociatePass());
  pass.add(llvm::createGVNPass());
  pass.add(llvm::createCFGSimplificationPass());
  pass.add(llvm::createAggressiveDCEPass());
  pass.add(llvm::createCFGSimplificationPass());
  pass.run(*mod);

#if PROFILE_ENABLED
  for (auto& func : *mod) {
    func.addFnAttr("frame-pointer", "all");
  }
#endif

  cantFail(jit_->addIRModule(
      llvm::orc::ThreadSafeModule(std::move(mod), std::move(context))));

  for (auto name : to_add) {
    compiled_fn_[name] = (void*)jit_->lookup(name)->getAddress();
  }
}

void* LLVMBackend::GetFunction(std::string_view name) {
  if (!compiled_fn_.contains(name)) {
#ifdef COMP_TIME
    auto t1 = std::chrono::high_resolution_clock::now();
#endif
    Translate(name);
#ifdef COMP_TIME
    auto t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> fp_ms = t2 - t1;
    compilation_time_ += fp_ms.count();
#endif
  }
  return compiled_fn_.at(name);
}

}  // namespace kush::khir