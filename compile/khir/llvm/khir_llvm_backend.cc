#include "compile/khir/llvm/khir_llvm_backend.h"

#include "llvm/ADT/iterator_range.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "compile/khir/instruction.h"
#include "compile/khir/khir_program_builder.h"
#include "compile/khir/khir_program_translator.h"
#include "compile/khir/type_manager.h"

namespace kush::khir {

KhirLLVMBackend::KhirLLVMBackend()
    : context_(std::make_unique<llvm::LLVMContext>()),
      module_(std::make_unique<llvm::Module>("query", *context_)),
      builder_(std::make_unique<llvm::IRBuilder<>>(*context_)) {}

void KhirLLVMBackend::TranslateVoidType() {
  types_.push_back(builder_->getVoidTy());
}

void KhirLLVMBackend::TranslateI1Type() {
  types_.push_back(builder_->getInt1Ty());
}

void KhirLLVMBackend::TranslateI8Type() {
  types_.push_back(builder_->getInt8Ty());
}

void KhirLLVMBackend::TranslateI16Type() {
  types_.push_back(builder_->getInt16Ty());
}

void KhirLLVMBackend::TranslateI32Type() {
  types_.push_back(builder_->getInt32Ty());
}

void KhirLLVMBackend::TranslateI64Type() {
  types_.push_back(builder_->getInt64Ty());
}

void KhirLLVMBackend::TranslateF64Type() {
  types_.push_back(builder_->getDoubleTy());
}

void KhirLLVMBackend::TranslatePointerType(Type elem) {
  types_.push_back(llvm::PointerType::get(types_[elem.GetID()], 0));
}

void KhirLLVMBackend::TranslateArrayType(Type elem, int len) {
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

void KhirLLVMBackend::TranslateFunctionType(Type result,
                                            absl::Span<const Type> arg_types) {
  types_.push_back(llvm::FunctionType::get(
      types_[result.GetID()], GetTypeArray(types_, arg_types), false));
}

void KhirLLVMBackend::TranslateStructType(absl::Span<const Type> elem_types) {
  types_.push_back(
      llvm::StructType::create(*context_, GetTypeArray(types_, elem_types)));
}

void KhirLLVMBackend::TranslateFuncDecl(bool external, std::string_view name,
                                        Type func_type) {
  std::string fn_name(name);
  auto type = llvm::dyn_cast<llvm::FunctionType>(types_[func_type.GetID()]);
  functions_.push_back(llvm::Function::Create(
      type,
      external ? llvm::GlobalValue::LinkageTypes::ExternalLinkage
               : llvm::GlobalValue::LinkageTypes::InternalLinkage,
      fn_name.c_str(), *module_));
}

void KhirLLVMBackend::TranslateFuncBody(
    int func_idx, const std::vector<uint64_t>& i64_constants,
    const std::vector<double>& f64_constants,
    const std::vector<int>& basic_block_order,
    const std::vector<std::pair<int, int>>& basic_blocks,
    const std::vector<uint64_t>& instructions) {
  // create basic blocks for each one
  llvm::Function* function = functions_[func_idx];
  std::vector<llvm::Value*> args;
  for (auto& a : function->args()) {
    args.push_back(&a);
  }

  std::vector<llvm::Value*> values(instructions.size(), nullptr);

  for (int i = 0; i < basic_blocks.size(); i++) {
    basic_blocks_.push_back(llvm::BasicBlock::Create(*context_, "", function));
  }

  for (int i : basic_block_order) {
    const auto& [i_start, i_end] = basic_blocks[i];

    builder_->SetInsertPoint(basic_blocks_[i]);
    for (int instr_idx = i_start; instr_idx <= i_end; instr_idx++) {
      TranslateInstr(args, i64_constants, f64_constants, values, instructions,
                     instr_idx);
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

void KhirLLVMBackend::TranslateInstr(const std::vector<llvm::Value*>& func_args,
                                     const std::vector<uint64_t>& i64_constants,
                                     const std::vector<double>& f64_constants,
                                     std::vector<llvm::Value*>& values,
                                     const std::vector<uint64_t>& instructions,
                                     int instr_idx) {
  auto instr = instructions[instr_idx];
  auto opcode = GenericInstructionReader(instr).Opcode();

  switch (opcode) {
    case Opcode::I1_CONST: {
      values[instr_idx] =
          builder_->getInt1(Type1InstructionReader(instr).Constant() == 1);
      return;
    }

    case Opcode::I8_CONST: {
      values[instr_idx] =
          builder_->getInt8(Type1InstructionReader(instr).Constant());
      return;
    }

    case Opcode::I16_CONST: {
      values[instr_idx] =
          builder_->getInt16(Type1InstructionReader(instr).Constant());
      return;
    }

    case Opcode::I32_CONST: {
      values[instr_idx] =
          builder_->getInt32(Type1InstructionReader(instr).Constant());
      return;
    }

    case Opcode::I64_CONST: {
      values[instr_idx] = builder_->getInt64(
          i64_constants[Type1InstructionReader(instr).Constant()]);
      return;
    }

    case Opcode::F64_CONST: {
      values[instr_idx] = llvm::ConstantFP::get(
          builder_->getDoubleTy(),
          f64_constants[Type1InstructionReader(instr).Constant()]);
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
      auto v0 = values[reader.Arg0()];
      auto v1 = values[reader.Arg1()];
      auto comp_type = GetLLVMCompType(opcode);
      values[instr_idx] = builder_->CreateCmp(comp_type, v0, v1);
      return;
    }

    case Opcode::I8_ADD:
    case Opcode::I16_ADD:
    case Opcode::I32_ADD:
    case Opcode::I64_ADD: {
      Type2InstructionReader reader(instr);
      auto v0 = values[reader.Arg0()];
      auto v1 = values[reader.Arg1()];
      values[instr_idx] = builder_->CreateAdd(v0, v1);
      return;
    }

    case Opcode::I8_MUL:
    case Opcode::I16_MUL:
    case Opcode::I32_MUL:
    case Opcode::I64_MUL: {
      Type2InstructionReader reader(instr);
      auto v0 = values[reader.Arg0()];
      auto v1 = values[reader.Arg1()];
      values[instr_idx] = builder_->CreateMul(v0, v1);
      return;
    }

    case Opcode::I8_SUB:
    case Opcode::I16_SUB:
    case Opcode::I32_SUB:
    case Opcode::I64_SUB: {
      Type2InstructionReader reader(instr);
      auto v0 = values[reader.Arg0()];
      auto v1 = values[reader.Arg1()];
      values[instr_idx] = builder_->CreateSub(v0, v1);
      return;
    }

    case Opcode::I8_DIV:
    case Opcode::I16_DIV:
    case Opcode::I32_DIV:
    case Opcode::I64_DIV: {
      Type2InstructionReader reader(instr);
      auto v0 = values[reader.Arg0()];
      auto v1 = values[reader.Arg1()];
      values[instr_idx] = builder_->CreateSDiv(v0, v1);
      return;
    }

    case Opcode::I1_ZEXT_I64:
    case Opcode::I8_ZEXT_I64:
    case Opcode::I16_ZEXT_I64:
    case Opcode::I32_ZEXT_I64: {
      Type2InstructionReader reader(instr);
      auto v = values[reader.Arg0()];
      values[instr_idx] = builder_->CreateZExt(v, builder_->getInt64Ty());
      return;
    }

    case Opcode::I1_LNOT: {
      Type2InstructionReader reader(instr);
      auto v = values[reader.Arg0()];
      values[instr_idx] = builder_->CreateXor(v, builder_->getInt1(true));
      return;
    }

    case Opcode::F64_ADD: {
      Type2InstructionReader reader(instr);
      auto v0 = values[reader.Arg0()];
      auto v1 = values[reader.Arg1()];
      values[instr_idx] = builder_->CreateFAdd(v0, v1);
      return;
    }

    case Opcode::F64_MUL: {
      Type2InstructionReader reader(instr);
      auto v0 = values[reader.Arg0()];
      auto v1 = values[reader.Arg1()];
      values[instr_idx] = builder_->CreateFMul(v0, v1);
      return;
    }

    case Opcode::F64_SUB: {
      Type2InstructionReader reader(instr);
      auto v0 = values[reader.Arg0()];
      auto v1 = values[reader.Arg1()];
      values[instr_idx] = builder_->CreateFSub(v0, v1);
      return;
    }

    case Opcode::F64_DIV: {
      Type2InstructionReader reader(instr);
      auto v0 = values[reader.Arg0()];
      auto v1 = values[reader.Arg1()];
      values[instr_idx] = builder_->CreateFDiv(v0, v1);
      return;
    }

    case Opcode::F64_CONV_I64: {
      Type2InstructionReader reader(instr);
      auto v = values[reader.Arg0()];
      values[instr_idx] = builder_->CreateFPToSI(v, builder_->getInt64Ty());
      return;
    }

    case Opcode::I64_CONV_F64: {
      Type2InstructionReader reader(instr);
      auto v = values[reader.Arg0()];
      values[instr_idx] = builder_->CreateSIToFP(v, builder_->getDoubleTy());
      return;
    }

    case Opcode::BR: {
      Type5InstructionReader reader(instr);
      auto bb = basic_blocks_[reader.Marg0()];
      values[instr_idx] = builder_->CreateBr(bb);
      return;
    }

    case Opcode::CONDBR: {
      Type5InstructionReader reader(instr);
      auto v = values[reader.Arg()];
      auto bb0 = basic_blocks_[reader.Marg0()];
      auto bb1 = basic_blocks_[reader.Marg1()];
      values[instr_idx] = builder_->CreateCondBr(v, bb0, bb1);
      return;
    }

    case Opcode::RETURN: {
      values[instr_idx] = builder_->CreateRetVoid();
      return;
    }

    case Opcode::RETURN_VALUE: {
      Type2InstructionReader reader(instr);
      auto v = values[reader.Arg0()];
      values[instr_idx] = builder_->CreateRet(v);
      return;
    }

    case Opcode::STORE: {
      Type2InstructionReader reader(instr);
      auto v0 = values[reader.Arg0()];
      auto v1 = values[reader.Arg1()];
      values[instr_idx] = builder_->CreateStore(v0, v1);
      return;
    }

    case Opcode::LOAD: {
      Type3InstructionReader reader(instr);
      auto v = values[reader.Arg()];
      values[instr_idx] = builder_->CreateLoad(v);
      return;
    }

    case Opcode::NULLPTR: {
      Type3InstructionReader reader(instr);
      auto t = types_[reader.TypeID()];
      values[instr_idx] =
          llvm::ConstantPointerNull::get(llvm::dyn_cast<llvm::PointerType>(t));
      return;
    }

    case Opcode::PTR_CAST: {
      Type3InstructionReader reader(instr);
      auto t = types_[reader.TypeID()];
      auto v = values[reader.Arg()];
      values[instr_idx] = builder_->CreatePointerCast(v, t);
      return;
    }

    case Opcode::PTR_ADD: {
      Type2InstructionReader reader(instr);
      auto ptr = values[reader.Arg0()];
      auto offset = values[reader.Arg1()];

      auto ptr_as_int = builder_->CreatePtrToInt(ptr, builder_->getInt64Ty());
      auto ptr_plus_offset = builder_->CreateAdd(ptr_as_int, offset);

      values[instr_idx] = builder_->CreateIntToPtr(
          ptr_plus_offset, llvm::PointerType::get(builder_->getVoidTy(), 0));
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
      auto v = values[reader.Arg()];
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
      auto func = functions_[reader.Arg()];
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

    case Opcode::PHI:
    case Opcode::STRING_GLOBAL_CONST:
    case Opcode::STRUCT_GLOBAL:
    case Opcode::ARRAY_GLOBAL:
    case Opcode::PTR_GLOBAL:
    case Opcode::PHI_MEMBER:
      throw std::runtime_error("Unimplemented.");
  }
}

}  // namespace kush::khir