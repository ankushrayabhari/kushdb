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

#include "khir/backend.h"
#include "khir/instruction.h"
#include "khir/llvm/perf_jit_event_listener.h"
#include "khir/type_manager.h"

namespace kush::khir {

LLVMBackend::LLVMBackend()
    : context_(std::make_unique<llvm::LLVMContext>()),
      module_(std::make_unique<llvm::Module>("query", *context_)),
      builder_(std::make_unique<llvm::IRBuilder<>>(*context_)) {}

LLVMBackend::~LLVMBackend() {}

void LLVMBackend::TranslateOpaqueType(std::string_view name) {
  types_.push_back(llvm::StructType::create(*context_, name));
}

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
    const Program& program) {
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
          program.I64Constants()[Type1InstructionReader(instr).Constant()]);
    }

    case ConstantOpcode::F64_CONST: {
      return llvm::ConstantFP::get(
          builder_->getDoubleTy(),
          program.F64Constants()[Type1InstructionReader(instr).Constant()]);
    }

    case ConstantOpcode::PTR_CONST: {
      auto i64_v = builder_->getInt64(reinterpret_cast<uint64_t>(
          program.PtrConstants()[Type1InstructionReader(instr).Constant()]));
      return llvm::ConstantExpr::getIntToPtr(i64_v, builder_->getInt8PtrTy());
    }

    case ConstantOpcode::PTR_CAST: {
      Type3InstructionReader reader(instr);
      auto t = types_[reader.TypeID()];
      auto v = constant_values[Value(reader.Arg()).GetIdx()];
      return llvm::ConstantExpr::getBitCast(v, t);
    }

    case ConstantOpcode::GLOBAL_CHAR_ARRAY_CONST: {
      return builder_->CreateGlobalStringPtr(
          program
              .CharArrayConstants()[Type1InstructionReader(instr).Constant()],
          "", 0, module_.get());
    }

    case ConstantOpcode::STRUCT_CONST: {
      const auto& x =
          program.StructConstants()[Type1InstructionReader(instr).Constant()];
      std::vector<llvm::Constant*> init;
      for (auto field_init : x.Fields()) {
        assert(field_init.IsConstantGlobal());
        init.push_back(constant_values[field_init.GetIdx()]);
      }
      auto* st = llvm::dyn_cast<llvm::StructType>(types_[x.Type().GetID()]);
      return llvm::ConstantStruct::get(st, init);
    }

    case ConstantOpcode::ARRAY_CONST: {
      const auto& x =
          program.ArrayConstants()[Type1InstructionReader(instr).Constant()];

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
      const auto& x =
          program.Globals()[Type1InstructionReader(instr).Constant()];

      auto type = types_[x.Type().GetID()];

      assert(x.InitialValue().IsConstantGlobal());
      auto init = constant_values[x.InitialValue().GetIdx()];

      return new llvm::GlobalVariable(
          *module_, type, false,
          llvm::GlobalValue::LinkageTypes::ExternalLinkage, init);
    }

    case ConstantOpcode::FUNC_PTR: {
      return functions_[Type3InstructionReader(instr).Arg()];
    }
  }
}

void LLVMBackend::Translate(const Program& program) {
  // Populate types_ array
  program.TypeManager().Translate(*this);

  // Translate all func decls
  for (const auto& func : program.Functions()) {
    std::string fn_name(func.Name());
    auto type = llvm::dyn_cast<llvm::FunctionType>(types_[func.Type().GetID()]);
    auto linkage = (func.External() || func.Public())
                       ? llvm::GlobalValue::LinkageTypes::ExternalLinkage
                       : llvm::GlobalValue::LinkageTypes::InternalLinkage;
    functions_.push_back(
        llvm::Function::Create(type, linkage, fn_name.c_str(), *module_));
  }

  // Convert all constants
  std::vector<llvm::Constant*> constant_values;
  for (auto instr : program.ConstantInstrs()) {
    constant_values.push_back(
        ConvertConstantInstr(instr, constant_values, program));
  }

  // Translate all func bodies
  for (int func_idx = 0; func_idx < program.Functions().size(); func_idx++) {
    const auto& func = program.Functions()[func_idx];
    if (func.External()) {
      external_functions_.emplace_back(func.Name(), func.Addr());
      continue;
    }

    if (func.Public()) {
      public_functions_.emplace_back(func.Name());
    }

    llvm::Function* function = functions_[func_idx];
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

    std::vector<llvm::BasicBlock*> basic_blocks_impl;
    for (int i = 0; i < basic_blocks.size(); i++) {
      basic_blocks_impl.push_back(
          llvm::BasicBlock::Create(*context_, "", function));
    }

    for (int i = 0; i < basic_blocks.size(); i++) {
      builder_->SetInsertPoint(basic_blocks_impl[i]);
      for (const auto& [b_start, b_end] : basic_blocks[i].Segments()) {
        for (int instr_idx = b_start; instr_idx <= b_end; instr_idx++) {
          TranslateInstr(program.TypeManager(), args, basic_blocks_impl, values,
                         constant_values, phi_member_list, instructions,
                         instr_idx);
        }
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
    const TypeManager& manager, const std::vector<llvm::Value*>& func_args,
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
    case Opcode::I1_AND:
    case Opcode::I64_AND: {
      Type2InstructionReader reader(instr);
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values);
      auto v1 = GetValue(Value(reader.Arg1()), constant_values, values);
      values[instr_idx] = builder_->CreateAnd(v0, v1);
      return;
    }

    case Opcode::I1_OR:
    case Opcode::I64_OR: {
      Type2InstructionReader reader(instr);
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values);
      auto v1 = GetValue(Value(reader.Arg1()), constant_values, values);
      values[instr_idx] = builder_->CreateOr(v0, v1);
      return;
    }

    case Opcode::I64_XOR: {
      Type2InstructionReader reader(instr);
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values);
      auto v1 = GetValue(Value(reader.Arg1()), constant_values, values);
      values[instr_idx] = builder_->CreateXor(v0, v1);
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

    case Opcode::PTR_CMP_NULLPTR: {
      Type2InstructionReader reader(instr);
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values);
      auto v1 = llvm::ConstantPointerNull::get(
          llvm::dyn_cast<llvm::PointerType>(v0->getType()));
      values[instr_idx] = builder_->CreateCmp(LLVMCmp::ICMP_EQ, v0, v1);
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

    case Opcode::I64_LSHIFT: {
      Type2InstructionReader reader(instr);
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values);
      auto v1 = GetValue(Value(reader.Arg1()), constant_values, values);
      values[instr_idx] = builder_->CreateShl(v0, v1);
      return;
    }

    case Opcode::I64_RSHIFT: {
      Type2InstructionReader reader(instr);
      auto v0 = GetValue(Value(reader.Arg0()), constant_values, values);
      auto v1 = GetValue(Value(reader.Arg1()), constant_values, values);
      values[instr_idx] = builder_->CreateLShr(v0, v1);
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

    case Opcode::I64_TRUNC_I16: {
      Type2InstructionReader reader(instr);
      auto v = GetValue(Value(reader.Arg0()), constant_values, values);
      values[instr_idx] = builder_->CreateTrunc(v, builder_->getInt16Ty());
      return;
    }

    case Opcode::I64_TRUNC_I32: {
      Type2InstructionReader reader(instr);
      auto v = GetValue(Value(reader.Arg0()), constant_values, values);
      values[instr_idx] = builder_->CreateTrunc(v, builder_->getInt32Ty());
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
      values[instr_idx] =
          builder_->CreateLoad(v->getType()->getPointerElementType(), v);
      return;
    }

    case Opcode::PTR_LOAD: {
      Type3InstructionReader reader(instr);
      auto v = GetValue(Value(reader.Arg()), constant_values, values);
      values[instr_idx] =
          builder_->CreateLoad(v->getType()->getPointerElementType(), v);
      return;
    }

    case Opcode::GEP: {
      Type3InstructionReader reader(instr);
      auto t = types_[reader.TypeID()];
      auto v = GetValue(Value(reader.Arg()), constant_values, values);
      values[instr_idx] = builder_->CreatePointerCast(v, t);
      return;
    }

    case Opcode::PTR_MATERIALIZE: {
      Type3InstructionReader reader(instr);
      values[instr_idx] =
          GetValue(Value(reader.Arg()), constant_values, values);
      return;
    }

    case Opcode::PTR_CAST: {
      Type3InstructionReader reader(instr);
      auto t = types_[reader.TypeID()];
      auto v = GetValue(Value(reader.Arg()), constant_values, values);
      values[instr_idx] = builder_->CreatePointerCast(v, t);
      return;
    }

    case Opcode::GEP_OFFSET: {
      Type2InstructionReader reader(instr);
      auto ptr = GetValue(Value(reader.Arg0()), constant_values, values);
      auto offset = GetValue(Value(reader.Arg1()), constant_values, values);

      auto ptr_as_int = builder_->CreatePtrToInt(ptr, builder_->getInt64Ty());
      auto ptr_plus_offset = builder_->CreateAdd(
          ptr_as_int, builder_->CreateZExt(offset, builder_->getInt64Ty()));

      values[instr_idx] =
          builder_->CreateIntToPtr(ptr_plus_offset, builder_->getInt8PtrTy());
      return;
    }

    case Opcode::ALLOCA: {
      Type3InstructionReader reader(instr);
      auto ptr_type = static_cast<Type>(reader.TypeID());
      auto elem_type = manager.GetPointerElementType(ptr_type);
      auto t = types_[elem_type.GetID()];
      int num_values = reader.Arg();
      values[instr_idx] =
          builder_->CreateAlloca(t, builder_->getInt32(num_values));
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
      auto func_type =
          llvm::dyn_cast<llvm::PointerType>(func->getType())->getElementType();
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

void LLVMBackend::Compile() {
  llvm::verifyModule(*module_, &llvm::errs());

  llvm::legacy::PassManager pass;
  pass.add(llvm::createInstructionCombiningPass());
  pass.add(llvm::createReassociatePass());
  pass.add(llvm::createGVNPass());
  pass.add(llvm::createCFGSimplificationPass());
  pass.add(llvm::createAggressiveDCEPass());
  pass.add(llvm::createCFGSimplificationPass());
  pass.run(*module_);

  auto target_triple = llvm::sys::getDefaultTargetTriple();
  module_->setTargetTriple(target_triple);

  std::string error;
  auto target = llvm::TargetRegistry::lookupTarget(target_triple, error);
  if (!target) {
    throw std::runtime_error("Target not found: " + error);
  }

#if PROFILE_ENABLED
  for (auto& func : *module_) {
    auto attrs = func.getAttributes().addAttribute(
        *context_, llvm::AttributeList::FunctionIndex, "frame-pointer", "all");
    func.setAttributes(attrs);
  }
#endif

  auto cpu = "x86-64";
  auto features = "";

  llvm::TargetOptions opt;
  auto reloc_model =
      llvm::Optional<llvm::Reloc::Model>(llvm::Reloc::Model::PIC_);
  auto target_machine = target->createTargetMachine(target_triple, cpu,
                                                    features, opt, reloc_model);
  module_->setDataLayout(target_machine->createDataLayout());

  jit_ = cantFail(
      llvm::orc::LLJITBuilder()
          .setDataLayout(module_->getDataLayout())
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

  cantFail(jit_->addIRModule(
      llvm::orc::ThreadSafeModule(std::move(module_), std::move(context_))));

  llvm::orc::SymbolMap symbol_map;
  for (const auto& [name, addr] : external_functions_) {
    auto mangled = jit_->mangleAndIntern(name);
    auto flags =
        llvm::JITSymbolFlags::Callable | llvm::JITSymbolFlags::Exported;
    auto symbol =
        llvm::JITEvaluatedSymbol(llvm::pointerToJITTargetAddress(addr), flags);
    symbol_map.try_emplace(mangled, symbol);
  }
  cantFail(
      jit_->getMainJITDylib().define(llvm::orc::absoluteSymbols(symbol_map)));

  // Trigger compilation for all public functions
  for (const auto& name : public_functions_) {
    compiled_fn_[name] = (void*)jit_->lookup(name)->getAddress();
  }
}

void* LLVMBackend::GetFunction(std::string_view name) const {
  return compiled_fn_.at(name);
}

}  // namespace kush::khir