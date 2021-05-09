#pragma once

#include <cstdint>

#include "compile/khir/opcode.h"

namespace kush::khir {

class GenericInstructionReader {
 public:
  GenericInstructionReader(uint64_t instr);
  Opcode Opcode() const;

 private:
  uint64_t instr_;
};

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
// [MD]                   RETURN

class Type1InstructionBuilder {
 public:
  Type1InstructionBuilder(uint64_t initial_instr = 0);
  Type1InstructionBuilder& SetMetadata(uint8_t metadata);
  Type1InstructionBuilder& SetConstant(uint64_t constant);
  Type1InstructionBuilder& SetOpcode(Opcode opcode);
  uint64_t Build();

 private:
  uint64_t value_;
};

class Type1InstructionReader {
 public:
  Type1InstructionReader(uint64_t instr);
  uint8_t Metadata() const;
  uint64_t Constant() const;
  Opcode Opcode() const;

 private:
  uint64_t instr_;
};

// Type II format:
//      8-bit METADATA,
//      [24-bit ARG] ** 2
//      8-bit opcode
// =============================================================================
// [MD] [ARG0] [ARG1] I1_CMP_EQ
// [MD] [ARG0] [ARG1] I1_CMP_NE
// [MD] [ARG0] [0]    I1_LNOT
// [MD] [ARG0] [0]    I1_ZEXT_I64
// [MD] [ARG0] [ARG1] I8_ADD
// [MD] [ARG0] [ARG1] I8_MUL
// [MD] [ARG0] [ARG1] I8_SUB
// [MD] [ARG0] [ARG1] I8_DIV
// [MD] [ARG0] [ARG1] I8_CMP_EQ
// [MD] [ARG0] [ARG1] I8_CMP_NE
// [MD] [ARG0] [ARG1] I8_CMP_GT
// [MD] [ARG0] [ARG1] I8_CMP_GE
// [MD] [ARG0] [ARG1] I8_CMP_LT
// [MD] [ARG0] [ARG1] I8_CMP_LE
// [MD] [ARG0] [0]    I8_ZEXT_I64
// [MD] [ARG0] [ARG1] I16_ADD
// [MD] [ARG0] [ARG1] I16_MUL
// [MD] [ARG0] [ARG1] I16_SUB
// [MD] [ARG0] [ARG1] I16_DIV
// [MD] [ARG0] [ARG1] I16_CMP_EQ
// [MD] [ARG0] [ARG1] I16_CMP_NE
// [MD] [ARG0] [ARG1] I16_CMP_GT
// [MD] [ARG0] [ARG1] I16_CMP_GE
// [MD] [ARG0] [ARG1] I16_CMP_LT
// [MD] [ARG0] [ARG1] I16_CMP_LE
// [MD] [ARG0] [0]    I16_ZEXT_I64
// [MD] [ARG0] [ARG1] I32_ADD
// [MD] [ARG0] [ARG1] I32_MUL
// [MD] [ARG0] [ARG1] I32_SUB
// [MD] [ARG0] [ARG1] I32_DIV
// [MD] [ARG0] [ARG1] I32_CMP_EQ
// [MD] [ARG0] [ARG1] I32_CMP_NE
// [MD] [ARG0] [ARG1] I32_CMP_GT
// [MD] [ARG0] [ARG1] I32_CMP_GE
// [MD] [ARG0] [ARG1] I32_CMP_LT
// [MD] [ARG0] [ARG1] I32_CMP_LE
// [MD] [ARG0] [0]    I32_ZEXT_I64
// [MD] [ARG0] [ARG1] I64_ADD
// [MD] [ARG0] [ARG1] I64_MUL
// [MD] [ARG0] [ARG1] I64_SUB
// [MD] [ARG0] [ARG1] I64_DIV
// [MD] [ARG0] [ARG1] I64_CMP_EQ
// [MD] [ARG0] [ARG1] I64_CMP_NE
// [MD] [ARG0] [ARG1] I64_CMP_GT
// [MD] [ARG0] [ARG1] I64_CMP_GE
// [MD] [ARG0] [ARG1] I64_CMP_LT
// [MD] [ARG0] [ARG1] I64_CMP_LE
// [MD] [ARG0] [0]    I64_CONV_F64
// [MD] [ARG0] [ARG1] F64_ADD
// [MD] [ARG0] [ARG1] F64_MUL
// [MD] [ARG0] [ARG1] F64_SUB
// [MD] [ARG0] [ARG1] F64_DIV
// [MD] [ARG0] [ARG1] F64_CMP_EQ
// [MD] [ARG0] [ARG1] F64_CMP_NE
// [MD] [ARG0] [ARG1] F64_CMP_GT
// [MD] [ARG0] [ARG1] F64_CMP_GE
// [MD] [ARG0] [ARG1] F64_CMP_LT
// [MD] [ARG0] [ARG1] F64_CMP_LE
// [MD] [ARG0] [0]    F64_CONV_I64
// [MD] [ARG0] [ARG1] STORE
// [MD] [ARG0] [0]    LOAD
// [MD] [ARG0] [0]    RETURN_VALUE
// [MD] [ARG0] [ARG1] CALL_EXT

class Type2InstructionBuilder {
 public:
  Type2InstructionBuilder(uint64_t initial_instr = 0);
  Type2InstructionBuilder& SetMetadata(uint8_t metadata);
  Type2InstructionBuilder& SetArg0(uint32_t arg0);
  Type2InstructionBuilder& SetArg1(uint32_t arg1);
  Type2InstructionBuilder& SetOpcode(Opcode opcode);
  uint64_t Build();

 private:
  uint64_t value_;
};

class Type2InstructionReader {
 public:
  Type2InstructionReader(uint64_t instr);
  uint8_t Metadata() const;
  uint32_t Arg0() const;
  uint32_t Arg1() const;
  Opcode Opcode() const;

 private:
  uint64_t instr_;
};

// Type III format:
//      8-bit METADATA
//      16-bit Type ID
//      8-bit SARG
//      24-bit ARG
//      8-bit opcode
// =============================================================================
// [MD] [ID] [SARG] [ARG] CALL
// - SARG: number of CALL_EXT
// [MD] [ID] [SARG] [0]   PHI
// - SARG: number of PHI_EXT
// [MD] [ID] [SARG] [ARG] GEP
// - SARG: number of GEP_EXT
// [MD] [ID] [0]    [ARG] PTR_CAST
// [MD] [ID] [0]          NULLPTR
// [MD] [ID] [0]          FUNC_ARG

class Type3InstructionBuilder {
 public:
  Type3InstructionBuilder(uint64_t initial_instr = 0);
  Type3InstructionBuilder& SetMetadata(uint8_t metadata);
  Type3InstructionBuilder& SetTypeID(uint16_t type_id);
  Type3InstructionBuilder& SetSarg(uint8_t sarg);
  Type3InstructionBuilder& SetArg(uint32_t arg);
  Type3InstructionBuilder& SetOpcode(Opcode opcode);
  uint64_t Build();

 private:
  uint64_t value_;
};

class Type3InstructionReader {
 public:
  Type3InstructionReader(uint64_t instr);
  uint8_t Metadata() const;
  uint16_t TypeID() const;
  uint8_t Sarg() const;
  uint32_t Arg() const;
  Opcode Opcode() const;

 private:
  uint64_t instr_;
};

// Type IV Format:
//      [8-bit SARG] ** 7
//      8-bit opcode
// =============================================================================
// [SARG0] [SARG1] [SARG2] [SARG3] [SARG4] [SARG5] [SARG6] GEP_EXT

class Type4InstructionBuilder {
 public:
  Type4InstructionBuilder(uint64_t initial_instr = 0);
  Type4InstructionBuilder& SetSarg(int8_t arg_idx, uint8_t sarg);
  Type4InstructionBuilder& SetOpcode(Opcode opcode);
  uint64_t Build();

 private:
  uint64_t value_;
};

class Type4InstructionReader {
 public:
  Type4InstructionReader(uint64_t instr);
  uint8_t Sarg(int8_t idx) const;
  Opcode Opcode() const;

 private:
  uint64_t instr_;
};

// Type V Format:
//      8-bit METADATA
//      24-bit ARG
//      12-bit MARG0
//      12-bit MARG1
//      8-bit opcode
// =============================================================================
// [MD] [0]   [SARG0] [0]     BR
// [MD] [ARG] [SARG0] [SARG1] CONDBR
// [MD] [ARG] [SARG0] [0]     PHI_EXT

class Type5InstructionBuilder {
 public:
  Type5InstructionBuilder(uint64_t initial_instr = 0);
  Type5InstructionBuilder& SetMetadata(uint8_t metadata);
  Type5InstructionBuilder& SetArg(uint32_t arg);
  Type5InstructionBuilder& SetMarg0(uint16_t marg0);
  Type5InstructionBuilder& SetMarg1(uint16_t marg1);
  Type5InstructionBuilder& SetOpcode(Opcode opcode);
  uint64_t Build();

 private:
  uint64_t value_;
};

class Type5InstructionReader {
 public:
  Type5InstructionReader(uint64_t instr);
  uint8_t Metadata() const;
  uint32_t Arg() const;
  uint16_t Marg0() const;
  uint16_t Marg1() const;
  Opcode Opcode() const;

 private:
  uint64_t instr_;
};

}  // namespace kush::khir