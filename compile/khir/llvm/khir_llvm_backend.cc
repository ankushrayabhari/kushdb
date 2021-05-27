#include "compile/khir/llvm/khir_llvm_backend.h"

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
  std::vector<llvm::Value*> values(instructions.size(), nullptr);

  for (int i = 0; i < basic_blocks.size(); i++) {
    basic_blocks_.push_back(llvm::BasicBlock::Create(*context_, "", function));
  }

  for (int i : basic_block_order) {
    const auto& [i_start, i_end] = basic_blocks[i];

    builder_->SetInsertPoint(basic_blocks_[i]);
    for (int instr_idx = i_start; instr_idx <= i_end; instr_idx++) {
      TranslateInstr(i64_constants, f64_constants, values, instructions,
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

void KhirLLVMBackend::TranslateInstr(const std::vector<uint64_t>& i64_constants,
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

    case Opcode::I1_LNOT:
    case Opcode::I8_ADD:
    case Opcode::I8_MUL:
    case Opcode::I8_SUB:
    case Opcode::I8_DIV:
    case Opcode::I16_ADD:
    case Opcode::I16_MUL:
    case Opcode::I16_SUB:
    case Opcode::I16_DIV:
    case Opcode::I32_ADD:
    case Opcode::I32_MUL:
    case Opcode::I32_SUB:
    case Opcode::I32_DIV:
    case Opcode::I64_ADD:
    case Opcode::I64_MUL:
    case Opcode::I64_SUB:
    case Opcode::I64_DIV:
    case Opcode::I1_ZEXT_I64:
    case Opcode::I8_ZEXT_I64:
    case Opcode::I16_ZEXT_I64:
    case Opcode::I32_ZEXT_I64:
    case Opcode::F64_CONV_I64:
    case Opcode::F64_ADD:
    case Opcode::F64_MUL:
    case Opcode::F64_SUB:
    case Opcode::F64_DIV:
    case Opcode::I64_CONV_F64:
    case Opcode::RETURN:
    case Opcode::STORE:
    case Opcode::RETURN_VALUE:
    case Opcode::CONDBR:
    case Opcode::BR:
    case Opcode::PHI:
    case Opcode::ALLOCA:
    case Opcode::CALL:
    case Opcode::CALL_INDIRECT:
    case Opcode::LOAD:
    case Opcode::PTR_CAST:
    case Opcode::FUNC_ARG:
    case Opcode::NULLPTR:
    case Opcode::STRING_GLOBAL_CONST:
    case Opcode::STRUCT_GLOBAL:
    case Opcode::ARRAY_GLOBAL:
    case Opcode::PTR_GLOBAL:
    case Opcode::PTR_ADD:
    case Opcode::PHI_MEMBER:
    case Opcode::CALL_ARG:
      throw std::runtime_error("Unimplemented.");
  }
}

}  // namespace kush::khir