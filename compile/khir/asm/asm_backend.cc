#include "compile/khir/asm/asm_backend.h"

#include <stdexcept>

#include "absl/types/span.h"

#include "asmjit/x86.h"

#include "compile/khir/instruction.h"
#include "compile/khir/program_builder.h"
#include "compile/khir/type_manager.h"

namespace kush::khir {

void ExceptionErrorHandler::handleError(asmjit::Error err, const char* message,
                                        asmjit::BaseEmitter* origin) {
  throw std::runtime_error(message);
}

ASMBackend::ASMBackend() {
  code_.init(rt_.environment());
  asm_ = std::make_unique<asmjit::x86::Assembler>(&code_);
  text_section_ = code_.textSection();
  code_.newSection(&data_section_, ".data", SIZE_MAX, 0, 8, 0);
}

void ASMBackend::TranslateGlobalConstCharArray(std::string_view s) {
  global_char_arrays_.push_back(asm_->newLabel());

  asm_->section(data_section_);
  asm_->bind(global_char_arrays_.back());

  for (char c : s) {
    asm_->embedUInt8(static_cast<uint8_t>(c));
  }
}

void ASMBackend::TranslateGlobalPointer(
    bool constant, Type t, uint64_t instr,
    const std::vector<uint64_t>& i64_constants,
    const std::vector<double>& f64_constants) {
  global_pointers_.push_back(asm_->newLabel());
  asm_->section(data_section_);
  asm_->bind(global_pointers_.back());

  auto opcode = GenericInstructionReader(instr).Opcode();
  switch (opcode) {
    case Opcode::NULLPTR: {
      asm_->embedUInt64(0);
      break;
    }

    case Opcode::I64_CONST: {
      auto init = i64_constants[Type1InstructionReader(instr).Constant()];
      asm_->embedUInt64(init);
      break;
    }

    case Opcode::CHAR_ARRAY_GLOBAL_CONST: {
      const auto& char_array_label =
          global_char_arrays_[Type1InstructionReader(instr).Constant()];
      asm_->embedLabel(char_array_label);
      break;
    }

    case Opcode::ARRAY_GLOBAL: {
      const auto& array_label =
          global_arrays_[Type1InstructionReader(instr).Constant()];
      asm_->embedLabel(array_label);
      break;
    }

    case Opcode::STRUCT_GLOBAL: {
      const auto& struct_label =
          global_structs_[Type1InstructionReader(instr).Constant()];
      asm_->embedLabel(struct_label);
      break;
    }

    case Opcode::PTR_GLOBAL: {
      const auto& ptr_label =
          global_pointers_[Type1InstructionReader(instr).Constant()];
      asm_->embedLabel(ptr_label);
      break;
    }

    default:
      throw std::runtime_error("Invalid constant for pointer.");
  }
}

}  // namespace kush::khir