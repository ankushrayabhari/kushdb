#include "compile/khir/instruction.h"

#include <cstdint>
#include <stdexcept>

#include "compile/khir/opcode.h"

namespace kush::khir {

void SetMetadataImpl(uint64_t& value, uint8_t metadata) {
  value = (value & ~(0xFFll << 56)) | static_cast<uint64_t>(metadata) << 56;
}

void SetOpcodeImpl(uint64_t& value, Opcode opcode) {
  uint8_t opcode_repr = static_cast<uint8_t>(opcode);
  value = (value & ~0xFFll) | static_cast<uint64_t>(opcode_repr);
}

// Type I format:
//      8-bit METADATA,
//      48-bit max-length, variable-length constant/ID
//      8-bit opcode
// =============================================================================
// [MD] [1-bit constant]  I1_CONST
// [MD] [8-bit constant]  I8_CONST
// [MD] [16-bit constant] I16_CONST
// [MD] [32-bit constant] I32_CONST
// [MD] [32-bit ID]       I64_CONST
// [MD] [32-bit ID]       F64_CONST
// [MD] [32-bit ID]       STR_CONST
// [MD]                  RETURN

Type1InstructionBuilder::Type1InstructionBuilder(uint64_t initial_instr)
    : value_(initial_instr) {}

Type1InstructionBuilder& Type1InstructionBuilder::SetMetadata(
    uint8_t metadata) {
  SetMetadataImpl(value_, metadata);
  return *this;
}

Type1InstructionBuilder& Type1InstructionBuilder::SetConstant(
    uint64_t constant) {
  const uint64_t constant_mask = (0xFFFFFFFFFFFFll << 8);
  value_ = (value_ & ~constant_mask) | ((constant << 8) & constant_mask);
  return *this;
}

Type1InstructionBuilder& Type1InstructionBuilder::SetOpcode(Opcode opcode) {
  SetOpcodeImpl(value_, opcode);
  return *this;
}

uint64_t Type1InstructionBuilder::Build() { return value_; }

// Type II format:
//      8-bit METADATA,
//      [24-bit ARG] ** 2
//      8-bit opcode
// =============================================================================
// [MD] [ARG0] [ARG1] I1_CMP
// [MD] [ARG0] [0]    I1_LNOT
// [MD] [ARG0] [0]    I1_ZEXT_I64
// [MD] [ARG0] [ARG1] I8_ADD
// [MD] [ARG0] [ARG1] I8_MUL
// [MD] [ARG0] [ARG1] I8_SUB
// [MD] [ARG0] [ARG1] I8_DIV
// [MD] [ARG0] [ARG1] I8_CMP
// [MD] [ARG0] [0]    I8_ZEXT_I64
// [MD] [ARG0] [ARG1] I16_ADD
// [MD] [ARG0] [ARG1] I16_MUL
// [MD] [ARG0] [ARG1] I16_SUB
// [MD] [ARG0] [ARG1] I16_DIV
// [MD] [ARG0] [ARG1] I16_CMP
// [MD] [ARG0] [0]    I16_ZEXT_I64
// [MD] [ARG0] [ARG1] I32_ADD
// [MD] [ARG0] [ARG1] I32_MUL
// [MD] [ARG0] [ARG1] I32_SUB
// [MD] [ARG0] [ARG1] I32_DIV
// [MD] [ARG0] [ARG1] I32_CMP
// [MD] [ARG0] [0]    I32_ZEXT_I64
// [MD] [ARG0] [ARG1] I64_ADD
// [MD] [ARG0] [ARG1] I64_MUL
// [MD] [ARG0] [ARG1] I64_SUB
// [MD] [ARG0] [ARG1] I64_DIV
// [MD] [ARG0] [ARG1] I64_CMP
// [MD] [ARG0] [0]    I64_CONV_F64
// [MD] [ARG0] [ARG1] F64_ADD
// [MD] [ARG0] [ARG1] F64_MUL
// [MD] [ARG0] [ARG1] F64_SUB
// [MD] [ARG0] [ARG1] F64_DIV
// [MD] [ARG0] [ARG1] F64_CMP
// [MD] [ARG0] [0]    F64_CONV_I64
// [MD] [ARG0] [ARG1] STORE
// [MD] [ARG0] [ARG1] CONDBR
// [MD] [ARG0] [0]    LOAD
// [MD] [ARG0] [0]    RETURN_VALUE
// [MD] [ARG0] [0]    BR
// [MD] [ARG0] [ARG1] PHI_EXT
// [MD] [ARG0] [ARG1] CALL_EXT

Type2InstructionBuilder::Type2InstructionBuilder(uint64_t initial_instr)
    : value_(initial_instr) {}

Type2InstructionBuilder& Type2InstructionBuilder::SetMetadata(
    uint8_t metadata) {
  SetMetadataImpl(value_, metadata);
  return *this;
}

Type2InstructionBuilder& Type2InstructionBuilder::SetArg0(uint32_t arg0) {
  uint64_t arg0_mask = 0xFFFFFFll << 32;
  value_ =
      (value_ & ~arg0_mask) | ((static_cast<uint64_t>(arg0) << 32) & arg0_mask);
  return *this;
}

Type2InstructionBuilder& Type2InstructionBuilder::SetArg1(uint32_t arg1) {
  uint64_t arg1_mask = 0xFFFFFFll << 8;
  value_ =
      (value_ & ~arg1_mask) | ((static_cast<uint64_t>(arg1) << 8) & arg1_mask);
  return *this;
}

Type2InstructionBuilder& Type2InstructionBuilder::SetOpcode(Opcode opcode) {
  SetOpcodeImpl(value_, opcode);
  return *this;
}

uint64_t Type2InstructionBuilder::Build() { return value_; }

// Type III format:
//      8-bit METADATA
//      16-bit Type ID
//      8-bit SARG
//      24-bit ARG
//      8-bit opcode
// =============================================================================
// [MD] [ID] [SARG] [ARG] CALL
// - SARG: number of CALL_EXT
// [MD] [ID] [SARG] [ARG] PHI
// - SARG: number of PHI_EXT
// [MD] [ID] [SARG] [ARG] GEP
// - SARG: number of GEP_EXT
// [MD] [ID] [0]    [ARG] PTR_CAST
// [MD] [ID] [0]          FUNC_ARG

Type3InstructionBuilder::Type3InstructionBuilder(uint64_t initial_instr)
    : value_(initial_instr) {}

Type3InstructionBuilder& Type3InstructionBuilder::SetMetadata(
    uint8_t metadata) {
  SetMetadataImpl(value_, metadata);
  return *this;
}

Type3InstructionBuilder& Type3InstructionBuilder::SetTypeID(uint16_t type_id) {
  uint64_t type_id_mask = 0xFFFFll << 40;
  value_ = (value_ & ~type_id_mask) |
           ((static_cast<uint64_t>(type_id) << 40) & type_id_mask);
  return *this;
}

Type3InstructionBuilder& Type3InstructionBuilder::SetSarg(uint8_t sarg) {
  uint64_t sarg_mask = 0xFFll << 32;
  value_ =
      (value_ & ~sarg_mask) | ((static_cast<uint64_t>(sarg) << 32) & sarg_mask);
  return *this;
}

Type3InstructionBuilder& Type3InstructionBuilder::SetArg(uint32_t arg) {
  uint64_t arg_mask = 0xFFFFFFll << 8;
  value_ =
      (value_ & ~arg_mask) | ((static_cast<uint64_t>(arg) << 8) & arg_mask);
  return *this;
}

Type3InstructionBuilder& Type3InstructionBuilder::SetOpcode(Opcode opcode) {
  SetOpcodeImpl(value_, opcode);
  return *this;
}

uint64_t Type3InstructionBuilder::Build() { return value_; }

// Type IV Format:
//      [8-bit SARG] ** 7
//      8-bit opcode
// =============================================================================
// [SARG0] [SARG1] [SARG2] [SARG3] [SARG4] [SARG5] [SARG6] GEP_EXT

Type4InstructionBuilder::Type4InstructionBuilder(uint64_t initial_instr)
    : value_(initial_instr) {}

Type4InstructionBuilder& Type4InstructionBuilder::SetSarg(int8_t arg_idx,
                                                          uint8_t sarg) {
  if (arg_idx >= 7) {
    throw std::runtime_error("arg idx must be less than 7");
  }

  int8_t byte = 6 - arg_idx + 1;
  uint64_t mask = 0xFFll << (byte * 8);
  value_ = (value_ & ~mask) | (static_cast<uint64_t>(sarg) << (byte * 8));
  return *this;
}

Type4InstructionBuilder& Type4InstructionBuilder::SetOpcode(Opcode opcode) {
  SetOpcodeImpl(value_, opcode);
  return *this;
}

uint64_t Type4InstructionBuilder::Build() { return value_; }

}  // namespace kush::khir