#pragma once

#include <cstdint>

#include "khir/opcode.h"

namespace kush::khir {

class GenericInstructionReader {
 public:
  GenericInstructionReader(uint64_t instr);
  uint8_t Opcode() const;

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
// [MD] [32-bit ID]       GLOBAL_CHAR_ARRAY_CONST
// [MD] [32-bit ID]       STRUCT_CONST
// [MD] [32-bit ID]       ARRAY_CONST
// [MD] [32-bit ID]       GLOBAL
// [MD]                   RETURN

class Type1InstructionBuilder {
 public:
  Type1InstructionBuilder(uint64_t initial_instr = 0);
  Type1InstructionBuilder& SetMetadata(uint8_t metadata);
  Type1InstructionBuilder& SetConstant(uint64_t constant);
  Type1InstructionBuilder& SetOpcode(uint8_t opcode);
  uint64_t Build();

 private:
  uint64_t value_;
};

class Type1InstructionReader {
 public:
  Type1InstructionReader(uint64_t instr);
  uint8_t Metadata() const;
  uint64_t Constant() const;
  uint8_t Opcode() const;

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
// [MD] [ARG0] [0]    I1_ZEXT_I8
// [MD] [ARG0] [0]    I1_ZEXT_I64
// [MD] [ARG0] [ARG1] I8_ADD
// [MD] [ARG0] [ARG1] I8_MUL
// [MD] [ARG0] [ARG1] I8_SUB
// [MD] [ARG0] [ARG1] I8_CMP_EQ
// [MD] [ARG0] [ARG1] I8_CMP_NE
// [MD] [ARG0] [ARG1] I8_CMP_GT
// [MD] [ARG0] [ARG1] I8_CMP_GE
// [MD] [ARG0] [ARG1] I8_CMP_LT
// [MD] [ARG0] [ARG1] I8_CMP_LE
// [MD] [ARG0] [0]    I8_ZEXT_I64
// [MD] [ARG0] [0]    I8_CONV_F64
// [MD] [ARG0] [ARG1] I16_ADD
// [MD] [ARG0] [ARG1] I16_MUL
// [MD] [ARG0] [ARG1] I16_SUB
// [MD] [ARG0] [ARG1] I16_CMP_EQ
// [MD] [ARG0] [ARG1] I16_CMP_NE
// [MD] [ARG0] [ARG1] I16_CMP_GT
// [MD] [ARG0] [ARG1] I16_CMP_GE
// [MD] [ARG0] [ARG1] I16_CMP_LT
// [MD] [ARG0] [ARG1] I16_CMP_LE
// [MD] [ARG0] [0]    I16_ZEXT_I64
// [MD] [ARG0] [0]    I16_CONV_F64
// [MD] [ARG0] [ARG1] I32_ADD
// [MD] [ARG0] [ARG1] I32_MUL
// [MD] [ARG0] [ARG1] I32_SUB
// [MD] [ARG0] [ARG1] I32_CMP_EQ
// [MD] [ARG0] [ARG1] I32_CMP_NE
// [MD] [ARG0] [ARG1] I32_CMP_GT
// [MD] [ARG0] [ARG1] I32_CMP_GE
// [MD] [ARG0] [ARG1] I32_CMP_LT
// [MD] [ARG0] [ARG1] I32_CMP_LE
// [MD] [ARG0] [0]    I32_ZEXT_I64
// [MD] [ARG0] [0]    I32_CONV_F64
// [MD] [ARG0] [ARG1] I64_ADD
// [MD] [ARG0] [ARG1] I64_MUL
// [MD] [ARG0] [ARG1] I64_SUB
// [MD] [ARG0] [ARG1] I64_AND
// [MD] [ARG0] [ARG1] I64_XOR
// [MD] [ARG0] [ARG1] I64_OR
// [MD] [ARG0] [ARG1] I64_LSHIFT
// [MD] [ARG0] [ARG1] I64_RSHIFT
// [MD] [ARG0] [ARG1] I64_CMP_EQ
// [MD] [ARG0] [ARG1] I64_CMP_NE
// [MD] [ARG0] [ARG1] I64_CMP_GT
// [MD] [ARG0] [ARG1] I64_CMP_GE
// [MD] [ARG0] [ARG1] I64_CMP_LT
// [MD] [ARG0] [ARG1] I64_CMP_LE
// [MD] [ARG0] [0]    I64_CONV_F64
// [MD] [ARG0] [0]    I64_TRUNC_I16
// [MD] [ARG0] [0]    I64_TRUNC_I32
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
// [MD] [ARG0] [ARG1] I8_STORE
// [MD] [ARG0] [ARG1] I16_STORE
// [MD] [ARG0] [ARG1] I32_STORE
// [MD] [ARG0] [ARG1] I64_STORE
// [MD] [ARG0] [ARG1] F64_STORE
// [MD] [ARG0] [ARG1] PTR_STORE
// [MD] [ARG0] [0]    I8_LOAD
// [MD] [ARG0] [0]    I16_LOAD
// [MD] [ARG0] [0]    I32_LOAD
// [MD] [ARG0] [0]    I64_LOAD
// [MD] [ARG0] [0]    F64_LOAD
// [MD] [ARG0] [ARG1] PHI_MEMBER
// [MD] [ARG0] [ARG1] GEP_STATIC_OFFSET
// [MD] [ARG0] [ARG1] GEP_DYNAMIC_OFFSET
// [MD] [ARG0] [ARG1] GEP_DYNAMIC_IDX

class Type2InstructionBuilder {
 public:
  Type2InstructionBuilder(uint64_t initial_instr = 0);
  Type2InstructionBuilder& SetMetadata(uint8_t metadata);
  Type2InstructionBuilder& SetArg0(uint32_t arg0);
  Type2InstructionBuilder& SetArg1(uint32_t arg1);
  Type2InstructionBuilder& SetOpcode(uint8_t opcode);
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
  uint8_t Opcode() const;

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
// [MD] [ID] [0]    [ARG] FUNC_PTR
// [MD] [ID] [0]    [ARG] CALL
// [MD] [ID] [0]    [ARG] CALL_INDIRECT
// [MD] [0]  [SARG] [ARG] CALL_ARG
// [MD] [ID] [0]    [ARG] PTR_CAST
// [MD] [ID] [0]    [0]   GEP_STATIC
// [MD] [ID] [0]    [ARG] GEP_DYNAMIC
// [MD] [ID] [0]          NULLPTR
// [MD] [ID] [SARG] [0]   FUNC_ARG
// [MD] [ID] [0]    [ARG] PTR_LOAD
// [MD] [ID] [0]          ALLOCA
// [MD] [ID] [0]          PHI
// [MD] [ID] [0]    [ARG] RETURN_VALUE

class Type3InstructionBuilder {
 public:
  Type3InstructionBuilder(uint64_t initial_instr = 0);
  Type3InstructionBuilder& SetMetadata(uint8_t metadata);
  Type3InstructionBuilder& SetTypeID(uint16_t type_id);
  Type3InstructionBuilder& SetSarg(uint8_t sarg);
  Type3InstructionBuilder& SetArg(uint32_t arg);
  Type3InstructionBuilder& SetOpcode(uint8_t opcode);
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
  uint8_t Opcode() const;

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
// [MD] [0]   [MARG0] [0]     BR
// [MD] [ARG] [MARG0] [MARG1] CONDBR

class Type5InstructionBuilder {
 public:
  Type5InstructionBuilder(uint64_t initial_instr = 0);
  Type5InstructionBuilder& SetMetadata(uint8_t metadata);
  Type5InstructionBuilder& SetArg(uint32_t arg);
  Type5InstructionBuilder& SetMarg0(uint16_t marg0);
  Type5InstructionBuilder& SetMarg1(uint16_t marg1);
  Type5InstructionBuilder& SetOpcode(uint8_t opcode);
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
  uint8_t Opcode() const;

 private:
  uint64_t instr_;
};

}  // namespace kush::khir