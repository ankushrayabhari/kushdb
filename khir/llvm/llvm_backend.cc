#include "khir/llvm/llvm_backend.h"

#include <dlfcn.h>
#include <iostream>
#include <system_error>
#include <type_traits>

#include "absl/types/span.h"

#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
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

#include "khir/instruction.h"
#include "khir/program_builder.h"
#include "khir/type_manager.h"

namespace kush::khir {

LLVMBackend::LLVMBackend()
    : context_(std::make_unique<llvm::LLVMContext>()),
      module_(std::make_unique<llvm::Module>("query", *context_)),
      builder_(std::make_unique<llvm::IRBuilder<>>(*context_)) {}

void LLVMBackend::TranslateVoidType() {
  types_.push_back(builder_->getVoidTy());
}

void LLVMBackend::TranslateI1Type() { types_.push_back(builder_->getInt1Ty()); }

void LLVMBackend::TranslateI8Type() { types_.push_back(builder_->getInt8Ty()); }

void LLVMBackend::TranslateI16Type() {
  types_.push_back(builder_->getInt16Ty());
}

void LLVMBackend::TranslateI32Type() {
  types_.push_back(builder_->getInt32Ty());
}

void LLVMBackend::TranslateI64Type() {
  types_.push_back(builder_->getInt64Ty());
}

void LLVMBackend::TranslateF64Type() {
  types_.push_back(builder_->getDoubleTy());
}

void LLVMBackend::TranslatePointerType(Type elem) {
  types_.push_back(llvm::PointerType::get(types_[elem.GetID()], 0));
}

void LLVMBackend::TranslateArrayType(Type elem, int len) {
  types_.push_back(llvm::ArrayType::get(types_[elem.GetID()], len));
}

std::vector<llvm::Type*> GetTypeArray(const std::vector<llvm::Type*>& types,
                                      absl::Span<const Type> elem_types) {
  std::vector<llvm::Type*> result;
  for (auto x : elem_types) {
    result.push_back(types[x.GetID()]);
  }
  return result;
}

void LLVMBackend::TranslateFunctionType(Type result,
                                        absl::Span<const Type> arg_types) {
  types_.push_back(llvm::FunctionType::get(
      types_[result.GetID()], GetTypeArray(types_, arg_types), false));
}

void LLVMBackend::TranslateStructType(absl::Span<const Type> elem_types) {
  types_.push_back(
      llvm::StructType::create(*context_, GetTypeArray(types_, elem_types)));
}

llvm::Constant* LLVMBackend::ConvertConstantInstr(
    uint64_t instr, std::vector<llvm::Constant*>& constant_values,
    const std::vector<uint64_t>& i64_constants,
    const std::vector<double>& f64_constants,
    const std::vector<std::string>& char_array_constants,
    const std::vector<StructConstant>& struct_constants,
    const std::vector<ArrayConstant>& array_constants,
    const std::vector<Global>& globals) {
  auto opcode = ConstantOpcodeFrom(GenericInstructionReader(instr).Opcode());

  switch (opcode) {
    case ConstantOpcode::I1_CONST: {
      return builder_->getInt1(Type1InstructionReader(instr).Constant() == 1);
    }

    case ConstantOpcode::I8_CONST: {
      return builder_->getInt8(Type1InstructionReader(instr).Constant());
    }

    case ConstantOpcode::I16_CONST: {
      return builder_->getInt16(Type1InstructionReader(instr).Constant());
    }

    case ConstantOpcode::I32_CONST: {
      return builder_->getInt32(Type1InstructionReader(instr).Constant());
    }

    case ConstantOpcode::I64_CONST: {
      return builder_->getInt64(
          i64_constants[Type1InstructionReader(instr).Constant()]);
    }

    case ConstantOpcode::F64_CONST: {
      return llvm::ConstantFP::get(
          builder_->getDoubleTy(),
          f64_constants[Type1InstructionReader(instr).Constant()]);
    }

    case ConstantOpcode::GLOBAL_CHAR_ARRAY_CONST: {
      return builder_->CreateGlobalStringPtr(
          char_array_constants[Type1InstructionReader(instr).Constant()], "", 0,
          module_.get());
    }

    case ConstantOpcode::STRUCT_CONST: {
      const auto& x =
          struct_constants[Type1InstructionReader(instr).Constant()];
      std::vector<llvm::Constant*> init;
      for (auto field_init : x.Fields()) {
        assert(field_init.IsConstantGlobal());
        init.push_back(constant_values[field_init.GetIdx()]);
      }
      auto* st = llvm::dyn_cast<llvm::StructType>(types_[x.Type().GetID()]);
      return llvm::ConstantStruct::get(st, init);
    }

    case ConstantOpcode::ARRAY_CONST: {
      const auto& x = array_constants[Type1InstructionReader(instr).Constant()];

      std::vector<llvm::Constant*> init;
      for (auto element_init : x.Elements()) {
        assert(element_init.IsConstantGlobal());
        init.push_back(constant_values[element_init.GetIdx()]);
      }
      auto* at = llvm::dyn_cast<llvm::ArrayType>(types_[x.Type().GetID()]);
      return llvm::ConstantArray::get(at, init);
    }

    case ConstantOpcode::NULLPTR: {
      auto t = types_[Type3InstructionReader(instr).TypeID()];
      return llvm::ConstantPointerNull::get(
          llvm::dyn_cast<llvm::PointerType>(t));
    }

    case ConstantOpcode::GLOBAL_REF: {
      const auto& x = globals[Type1InstructionReader(instr).Constant()];

      auto type = types_[x.Type().GetID()];

      assert(x.InitialValue().IsConstantGlobal());
      auto init = constant_values[x.InitialValue().GetIdx()];

      return new llvm::GlobalVariable(
          *module_, type, x.Constant(),
          x.Public() ? llvm::GlobalValue::LinkageTypes::ExternalLinkage
                     : llvm::GlobalValue::LinkageTypes::InternalLinkage,
          init);
    }

    default:
      throw std::runtime_error("Invalid constant.");
  }
}

void LLVMBackend::Translate(
    const TypeManager& manager, const std::vector<uint64_t>& i64_constants,
    const std::vector<double>& f64_constants,
    const std::vector<std::string>& char_array_constants,
    const std::vector<StructConstant>& struct_constants,
    const std::vector<ArrayConstant>& array_constants,
    const std::vector<Global>& globals,
    const std::vector<uint64_t>& constant_instrs,
    const std::vector<Function>& functions) {
  start = std::chrono::system_clock::now();

  // Populate types_ array
  manager.Translate(*this);

  // Convert all constants
  std::vector<llvm::Constant*> constant_values(constant_instrs.size(), nullptr);
  for (auto instr : constant_instrs) {
    constant_values.push_back(ConvertConstantInstr(
        instr, constant_values, i64_constants, f64_constants,
        char_array_constants, struct_constants, array_constants, globals));
  }

  // Translate all func decls
  for (const auto& func : functions) {
    std::string fn_name(func.Name());
    auto type = llvm::dyn_cast<llvm::FunctionType>(types_[func.Type().GetID()]);
    auto linkage = (func.External() || func.Public())
                       ? llvm::GlobalValue::LinkageTypes::ExternalLinkage
                       : llvm::GlobalValue::LinkageTypes::InternalLinkage;
    functions_.push_back(
        llvm::Function::Create(type, linkage, fn_name.c_str(), *module_));
  }

  // Translate all func bodies
  for (int func_idx = 0; func_idx < functions.size(); func_idx++) {
    const auto& func = functions[func_idx];
    if (func.External()) {
      continue;
    }

    llvm::Function* function = functions_[func_idx];
    std::vector<llvm::Value*> args;
    for (auto& a : function->args()) {
      args.push_back(&a);
    }

    const auto& instructions = func.Instructions();
    const auto& basic_blocks = func.BasicBlocks();
    const auto& basic_block_order = func.BasicBlockOrder();

    absl::flat_hash_map<uint32_t,
                        std::vector<std::pair<llvm::Value*, llvm::BasicBlock*>>>
        phi_member_list;
    std::vector<llvm::Value*> values(instructions.size(), nullptr);

    std::vector<llvm::BasicBlock*> basic_blocks_impl;
    for (int i = 0; i < basic_blocks.size(); i++) {
      basic_blocks_impl.push_back(
          llvm::BasicBlock::Create(*context_, "", function));
    }

    for (int i : basic_block_order) {
      const auto& [i_start, i_end] = basic_blocks[i];

      builder_->SetInsertPoint(basic_blocks_impl[i]);
      for (int instr_idx = i_start; instr_idx <= i_end; instr_idx++) {
        TranslateInstr(args, basic_blocks_impl, values, constant_values,
                       phi_member_list, instructions, instr_idx);
      }
    }
  }
}

using LLVMCmp = llvm::CmpInst::Predicate;

LLVMCmp GetLLVMCompType(Opcode opcode) {
  switch (opcode) {
    case Opcode::I1_CMP_EQ:
    case Opcode::I8_CMP_EQ:
    case Opcode::I16_CMP_EQ:
    case Opcode::I32_CMP_EQ:
    case Opcode::I64_CMP_EQ:
      return LLVMCmp::ICMP_EQ;

    case Opcode::I1_CMP_NE:
    case Opcode::I8_CMP_NE:
    case Opcode::I16_CMP_NE:
    case Opcode::I32_CMP_NE:
    case Opcode::I64_CMP_NE:
      return LLVMCmp::ICMP_NE;

    case Opcode::I8_CMP_LT:
    case Opcode::I16_CMP_LT:
    case Opcode::I32_CMP_LT:
    case Opcode::I64_CMP_LT:
      return LLVMCmp::ICMP_SLT;

    case Opcode::I8_CMP_LE:
    case Opcode::I16_CMP_LE:
    case Opcode::I32_CMP_LE:
    case Opcode::I64_CMP_LE:
      return LLVMCmp::ICMP_SLE;

    case Opcode::I8_CMP_GT:
    case Opcode::I16_CMP_GT:
    case Opcode::I32_CMP_GT:
    case Opcode::I64_CMP_GT:
      return LLVMCmp::ICMP_SGT;

    case Opcode::I8_CMP_GE:
    case Opcode::I16_CMP_GE:
    case Opcode::I32_CMP_GE:
    case Opcode::I64_CMP_GE:
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

llvm::Value* GetValue(khir::Value v,
                      const std::vector<llvm::Constant*>& constant_values,
                      const std::vector<llvm::Value*>& values) {
  if (v.IsConstantGlobal()) {
    return constant_values[v.GetIdx()];
  } else {
    return values[v.GetIdx()];
  }
}

void LLVMBackend::TranslateInstr(
    const std::vector<llvm::Value*>& func_args,
    const std::vector<llvm::BasicBlock*>& basic_blocks,
    std::vector<llvm::Value*>& values,
    const std::vector<llvm::Constant*>& constant_values,
    absl::flat_hash_map<
        uint32_t, std::vector<std::pair<llvm::Value*, llvm::BasicBlock*>>>&
        phi_member_list,
    const std::vector<uint64_t>& instructions, int instr_idx) {
  auto instr = instructions[instr_idx];
  auto opcode = OpcodeFrom(GenericInstructionReader(instr).Opcode());

  switch (opcode) {
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
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values);
      auto v1 = GetValue(Value(reader.Arg1()), constant_values, values);
      auto comp_type = GetLLVMCompType(opcode);
      values[instr_idx] = builder_->CreateCmp(comp_type, v0, v1);
      return;
    }

    case Opcode::I8_ADD:
    case Opcode::I16_ADD:
    case Opcode::I32_ADD:
    case Opcode::I64_ADD: {
      Type2InstructionReader reader(instr);
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values);
      auto v1 = GetValue(Value(reader.Arg1()), constant_values, values);
      values[instr_idx] = builder_->CreateAdd(v0, v1);
      return;
    }

    case Opcode::I8_MUL:
    case Opcode::I16_MUL:
    case Opcode::I32_MUL:
    case Opcode::I64_MUL: {
      Type2InstructionReader reader(instr);
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values);
      auto v1 = GetValue(Value(reader.Arg1()), constant_values, values);
      values[instr_idx] = builder_->CreateMul(v0, v1);
      return;
    }

    case Opcode::I8_SUB:
    case Opcode::I16_SUB:
    case Opcode::I32_SUB:
    case Opcode::I64_SUB: {
      Type2InstructionReader reader(instr);
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values);
      auto v1 = GetValue(Value(reader.Arg1()), constant_values, values);
      values[instr_idx] = builder_->CreateSub(v0, v1);
      return;
    }

    case Opcode::I8_DIV:
    case Opcode::I16_DIV:
    case Opcode::I32_DIV:
    case Opcode::I64_DIV: {
      Type2InstructionReader reader(instr);
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values);
      auto v1 = GetValue(Value(reader.Arg1()), constant_values, values);
      values[instr_idx] = builder_->CreateSDiv(v0, v1);
      return;
    }

    case Opcode::I1_ZEXT_I8: {
      Type2InstructionReader reader(instr);
      auto v = GetValue(Value(reader.Arg0()), constant_values, values);
      values[instr_idx] = builder_->CreateZExt(v, builder_->getInt8Ty());
      return;
    }

    case Opcode::I1_ZEXT_I64:
    case Opcode::I8_ZEXT_I64:
    case Opcode::I16_ZEXT_I64:
    case Opcode::I32_ZEXT_I64: {
      Type2InstructionReader reader(instr);
      auto v = GetValue(Value(reader.Arg0()), constant_values, values);
      values[instr_idx] = builder_->CreateZExt(v, builder_->getInt64Ty());
      return;
    }

    case Opcode::I1_LNOT: {
      Type2InstructionReader reader(instr);
      auto v = GetValue(Value(reader.Arg0()), constant_values, values);
      values[instr_idx] = builder_->CreateXor(v, builder_->getInt1(true));
      return;
    }

    case Opcode::F64_ADD: {
      Type2InstructionReader reader(instr);
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values);
      auto v1 = GetValue(Value(reader.Arg1()), constant_values, values);
      values[instr_idx] = builder_->CreateFAdd(v0, v1);
      return;
    }

    case Opcode::F64_MUL: {
      Type2InstructionReader reader(instr);
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values);
      auto v1 = GetValue(Value(reader.Arg1()), constant_values, values);
      values[instr_idx] = builder_->CreateFMul(v0, v1);
      return;
    }

    case Opcode::F64_SUB: {
      Type2InstructionReader reader(instr);
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values);
      auto v1 = GetValue(Value(reader.Arg1()), constant_values, values);
      values[instr_idx] = builder_->CreateFSub(v0, v1);
      return;
    }

    case Opcode::F64_DIV: {
      Type2InstructionReader reader(instr);
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values);
      auto v1 = GetValue(Value(reader.Arg1()), constant_values, values);
      values[instr_idx] = builder_->CreateFDiv(v0, v1);
      return;
    }

    case Opcode::F64_CONV_I64: {
      Type2InstructionReader reader(instr);
      auto v = GetValue(Value(reader.Arg0()), constant_values, values);
      values[instr_idx] = builder_->CreateFPToSI(v, builder_->getInt64Ty());
      return;
    }

    case Opcode::I8_CONV_F64:
    case Opcode::I16_CONV_F64:
    case Opcode::I32_CONV_F64:
    case Opcode::I64_CONV_F64: {
      Type2InstructionReader reader(instr);
      auto v = GetValue(Value(reader.Arg0()), constant_values, values);
      values[instr_idx] = builder_->CreateSIToFP(v, builder_->getDoubleTy());
      return;
    }

    case Opcode::BR: {
      Type5InstructionReader reader(instr);
      auto bb = basic_blocks[reader.Marg0()];
      values[instr_idx] = builder_->CreateBr(bb);
      return;
    }

    case Opcode::CONDBR: {
      Type5InstructionReader reader(instr);
      auto v = GetValue(Value(reader.Arg()), constant_values, values);
      auto bb0 = basic_blocks[reader.Marg0()];
      auto bb1 = basic_blocks[reader.Marg1()];
      values[instr_idx] = builder_->CreateCondBr(v, bb0, bb1);
      return;
    }

    case Opcode::RETURN: {
      values[instr_idx] = builder_->CreateRetVoid();
      return;
    }

    case Opcode::RETURN_VALUE: {
      Type3InstructionReader reader(instr);
      auto v = GetValue(Value(reader.Arg()), constant_values, values);
      values[instr_idx] = builder_->CreateRet(v);
      return;
    }

    case Opcode::I8_STORE:
    case Opcode::I16_STORE:
    case Opcode::I32_STORE:
    case Opcode::I64_STORE:
    case Opcode::F64_STORE:
    case Opcode::PTR_STORE: {
      Type2InstructionReader reader(instr);
      auto ptr = GetValue(Value(reader.Arg0()), constant_values, values);
      auto val = GetValue(Value(reader.Arg1()), constant_values, values);
      values[instr_idx] = builder_->CreateStore(val, ptr);
      return;
    }

    case Opcode::I8_LOAD:
    case Opcode::I16_LOAD:
    case Opcode::I32_LOAD:
    case Opcode::I64_LOAD:
    case Opcode::F64_LOAD: {
      Type2InstructionReader reader(instr);
      auto v = GetValue(Value(reader.Arg0()), constant_values, values);
      values[instr_idx] = builder_->CreateLoad(v);
      return;
    }

    case Opcode::PTR_LOAD: {
      Type3InstructionReader reader(instr);
      auto v = GetValue(Value(reader.Arg()), constant_values, values);
      values[instr_idx] = builder_->CreateLoad(v);
      return;
    }

    case Opcode::FUNC_PTR: {
      Type3InstructionReader reader(instr);
      values[instr_idx] = functions_[reader.Arg()];
      return;
    }

    case Opcode::PTR_CAST: {
      Type3InstructionReader reader(instr);
      auto t = types_[reader.TypeID()];
      auto v = GetValue(Value(reader.Arg()), constant_values, values);
      values[instr_idx] = builder_->CreatePointerCast(v, t);
      return;
    }

    case Opcode::PTR_ADD: {
      Type2InstructionReader reader(instr);
      auto ptr = GetValue(Value(reader.Arg0()), constant_values, values);
      auto offset = GetValue(Value(reader.Arg1()), constant_values, values);

      auto ptr_as_int = builder_->CreatePtrToInt(ptr, builder_->getInt64Ty());
      auto ptr_plus_offset = builder_->CreateAdd(ptr_as_int, offset);

      values[instr_idx] =
          builder_->CreateIntToPtr(ptr_plus_offset, builder_->getInt8PtrTy());
      return;
    }

    case Opcode::ALLOCA: {
      Type3InstructionReader reader(instr);
      auto t = types_[reader.TypeID()];
      values[instr_idx] = builder_->CreateAlloca(t);
      return;
    }

    case Opcode::CALL_ARG: {
      Type3InstructionReader reader(instr);
      auto v = GetValue(Value(reader.Arg()), constant_values, values);
      call_args_.push_back(v);
      return;
    }

    case Opcode::CALL: {
      Type3InstructionReader reader(instr);
      auto func = functions_[reader.Arg()];
      values[instr_idx] = builder_->CreateCall(func, call_args_);
      call_args_.clear();
      return;
    }

    case Opcode::CALL_INDIRECT: {
      Type3InstructionReader reader(instr);
      auto func = GetValue(Value(reader.Arg()), constant_values, values);
      auto func_type = types_[reader.TypeID()];
      values[instr_idx] = llvm::CallInst::Create(
          llvm::dyn_cast<llvm::FunctionType>(func_type), func, call_args_, "",
          builder_->GetInsertBlock());
      call_args_.clear();
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
      auto phi = GetValue(Value(reader.Arg0()), constant_values, values);
      auto phi_member = GetValue(Value(reader.Arg1()), constant_values, values);

      if (phi == nullptr) {
        phi_member_list[reader.Arg0()].emplace_back(phi_member,
                                                    builder_->GetInsertBlock());
      } else {
        llvm::dyn_cast<llvm::PHINode>(phi)->addIncoming(
            phi_member, builder_->GetInsertBlock());
      }
      return;
    }

    case Opcode::PHI: {
      Type3InstructionReader reader(instr);
      auto t = types_[reader.TypeID()];

      auto phi = builder_->CreatePHI(t, 2);
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

void LLVMBackend::Execute() {
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
          "clang++ -shared -fpic bazel-bin/runtime/libprinter.so "
          "bazel-bin/runtime/libstring.so "
          "bazel-bin/runtime/libcolumn_data.so "
          "bazel-bin/runtime/libvector.so bazel-bin/runtime/libhash_table.so "
          "bazel-bin/runtime/libtuple_idx_table.so "
          "bazel-bin/runtime/libcolumn_index.so "
          "bazel-bin/runtime/libskinner_join_executor.so "
          "/tmp/query.o -o /tmp/query.so")) {
    throw std::runtime_error("Failed to link file.");
  }

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
  comp = std::chrono::system_clock::now();

  process_query();
  dlclose(handle);
  end = std::chrono::system_clock::now();

  std::cerr << "Performance stats (ms):" << std::endl;
  std::chrono::duration<double, std::milli> elapsed_seconds = comp - start;
  std::cerr << "Compilation: " << elapsed_seconds.count() << std::endl;
  elapsed_seconds = end - comp;
  std::cerr << "Execution: " << elapsed_seconds.count() << std::endl;
}

}  // namespace kush::khir