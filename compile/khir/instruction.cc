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