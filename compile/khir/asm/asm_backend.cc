#include "compile/khir/asm/asm_backend.h"

#include <stdexcept>

#include "absl/types/span.h"

#include "asmjit/x86.h"

#include "compile/khir/instruction.h"
#include "compile/khir/program_builder.h"
#include "compile/khir/type_manager.h"

namespace kush::khir {

using namespace asmjit;

void ExceptionErrorHandler::handleError(Error err, const char* message,
                                        BaseEmitter* origin) {
  throw std::runtime_error(message);
}

uint64_t ASMBackend::OutputConstant(
    uint64_t instr, const TypeManager& type_manager,
    const std::vector<uint64_t>& i64_constants,
    const std::vector<double>& f64_constants,
    const std::vector<std::string>& char_array_constants,
    const std::vector<StructConstant>& struct_constants,
    const std::vector<ArrayConstant>& array_constants,
    const std::vector<Global>& globals) {
  auto opcode = GenericInstructionReader(instr).Opcode();

  switch (opcode) {
    case Opcode::I1_CONST:
    case Opcode::I8_CONST: {
      // treat I1 as I8
      auto constant = Type1InstructionReader(instr).Constant() & 0xFF;
      asm_->embedUInt8(static_cast<uint8_t>(constant));
      return 1;
    }

    case Opcode::I16_CONST: {
      auto constant = Type1InstructionReader(instr).Constant() & 0xFFFF;
      asm_->embedUInt16(static_cast<uint16_t>(constant));
      return 2;
    }

    case Opcode::I32_CONST: {
      auto constant = Type1InstructionReader(instr).Constant() & 0xFFFFFFFF;
      asm_->embedUInt32(static_cast<uint32_t>(constant));
      return 4;
    }

    case Opcode::I64_CONST: {
      auto i64_id = Type1InstructionReader(instr).Constant();
      asm_->embedUInt64(i64_constants[i64_id]);
      return 8;
    }

    case Opcode::F64_CONST: {
      auto f64_id = Type1InstructionReader(instr).Constant();
      asm_->embedDouble(f64_constants[f64_id]);
      return 8;
    }

    case Opcode::GLOBAL_CHAR_ARRAY_CONST: {
      auto str_id = Type1InstructionReader(instr).Constant();
      asm_->embedLabel(char_array_constants_[str_id]);
      return 8;
    }

    case Opcode::NULLPTR: {
      asm_->embedUInt64(0);
      return 8;
    }

    case Opcode::GLOBAL_REF: {
      auto global_id = Type1InstructionReader(instr).Constant();
      asm_->embedLabel(globals_[global_id]);
      return 8;
    }

    case Opcode::STRUCT_CONST: {
      auto struct_id = Type1InstructionReader(instr).Constant();
      const auto& struct_const = struct_constants[struct_id];

      auto field_offsets =
          type_manager.GetStructFieldOffsets(struct_const.Type());
      auto type_size = type_manager.GetTypeSize(struct_const.Type());
      auto field_constants = struct_const.Fields();

      uint64_t bytes_written = 0;

      for (int i = 0; i < field_constants.size(); i++) {
        auto field_const = field_constants[i];

        // add any padding until we're at the offset
        while (bytes_written < field_offsets[i]) {
          asm_->embedUInt8(0);
          bytes_written++;
        }

        bytes_written += OutputConstant(
            field_const, type_manager, i64_constants, f64_constants,
            char_array_constants, struct_constants, array_constants, globals);
      }

      while (bytes_written < type_size) {
        asm_->embedUInt8(0);
        bytes_written++;
      }

      return bytes_written;
    }

    case Opcode::ARRAY_CONST: {
      auto arr_id = Type1InstructionReader(instr).Constant();
      const auto& arr_const = array_constants[arr_id];

      uint64_t bytes_written = 0;
      for (auto elem_const : arr_const.Elements()) {
        // arrays have no padding
        bytes_written += OutputConstant(
            elem_const, type_manager, i64_constants, f64_constants,
            char_array_constants, struct_constants, array_constants, globals);
      }
      return bytes_written;
    }

    default:
      throw std::runtime_error("Invalid constant.");
  }
}

void ASMBackend::Translate(const TypeManager& type_manager,
                           const std::vector<uint64_t>& i64_constants,
                           const std::vector<double>& f64_constants,
                           const std::vector<std::string>& char_array_constants,
                           const std::vector<StructConstant>& struct_constants,
                           const std::vector<ArrayConstant>& array_constants,
                           const std::vector<Global>& globals,
                           const std::vector<Function>& functions) {
  code_.init(rt_.environment());
  asm_ = std::make_unique<x86::Assembler>(&code_);

  Section* text_section = code_.textSection();
  Section* data_section;
  code_.newSection(&data_section, ".data", SIZE_MAX, 0, 8, 0);

  asm_->section(data_section);

  // Write out all string constants
  for (const auto& str : char_array_constants) {
    char_array_constants_.push_back(asm_->newLabel());
    asm_->bind(char_array_constants_.back());
    for (char c : str) {
      asm_->embedUInt8(static_cast<uint8_t>(c));
    }
  }

  // Write out all global variables
  for (const auto& global : globals) {
    globals_.push_back(asm_->newLabel());
    asm_->bind(globals_.back());
    OutputConstant(global.InitialValue(), type_manager, i64_constants,
                   f64_constants, char_array_constants, struct_constants,
                   array_constants, globals);
  }

  asm_->section(text_section);

  // Declare all functions
  for (const auto& function : functions) {
    external_func_addr_.push_back(function.Addr());
    internal_func_labels_.push_back(asm_->newLabel());
  }

  const std::vector<x86::Gpq> callee_saved_registers = {
      x86::rbx, x86::r12, x86::r13, x86::r14, x86::r15};

  // Translate all function bodies
  for (int func_idx = 0; func_idx < functions.size(); func_idx++) {
    const auto& func = functions[func_idx];
    if (func.External()) {
      continue;
    }

    asm_->bind(internal_func_labels_[func_idx]);

    // Prologue ================================================================
    // - Save all callee saved registers
    for (auto reg : callee_saved_registers) {
      asm_->push(reg);
    }
    // - Save RBP and Store RSP in RBP
    asm_->push(x86::rbp);
    asm_->mov(x86::rbp, x86::rsp);

    // - Allocate placeholder stack space.
    auto stack_alloc_offset = asm_->offset();
    asm_->long_().sub(x86::rsp, 0);
    // =========================================================================

    const auto& instructions = func.Instructions();
    const auto& basic_blocks = func.BasicBlocks();
    const auto& basic_block_order = func.BasicBlockOrder();

    std::vector<Label> basic_blocks_impl;
    Label epilogue = asm_->newLabel();
    for (int i = 0; i < basic_blocks.size(); i++) {
      basic_blocks_impl.push_back(asm_->newLabel());
    }

    uint64_t stack_size = 0;
    /*
    for (int i : basic_block_order) {
      const auto& [i_start, i_end] = basic_blocks[i];
      for (int instr_idx = i_start; instr_idx <= i_end; instr_idx++) {
        stack_size += TranslateInstr(args, basic_blocks_impl, epilogue,
                                     instructions, instr_idx);
      }
    }
    */
    // Clobber all registers as a test
    for (auto reg :
         {x86::rbx, x86::r12, x86::r13, x86::r14, x86::r15, x86::rax, x86::rcx,
          x86::rdx, x86::rsi, x86::rdi, x86::r8, x86::r9, x86::r10, x86::r11}) {
      asm_->xor_(reg, reg);
    }

    // Update prologue to contain stack_size
    auto epilogue_offset = asm_->offset();
    asm_->setOffset(stack_alloc_offset);
    asm_->long_().sub(x86::rsp, stack_size);
    asm_->setOffset(epilogue_offset);

    // Epilogue ================================================================
    // - Restore RBP and dealloc stack space
    asm_->bind(epilogue);
    asm_->leave();

    // - Restore callee saved registers
    for (int i = callee_saved_registers.size() - 1; i >= 0; i--) {
      asm_->pop(callee_saved_registers[i]);
    }

    // - Return
    asm_->ret();
    // =========================================================================
  }
}

void ASMBackend::Execute() const {
  // TODO
}

}  // namespace kush::khir