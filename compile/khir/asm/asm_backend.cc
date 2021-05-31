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
  asm_ = std::make_unique<asmjit::x86::Assembler>(&code_);
  text_section_ = code_.textSection();
  code_.newSection(&data_section_, ".data", SIZE_MAX, 0, 8, 0);

  asm_->section(data_section_);

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
}

void ASMBackend::Compile() const {
  // TODO
}

void ASMBackend::Execute() const {
  // TODO
}

}  // namespace kush::khir