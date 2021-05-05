#pragma once

// Primitive system types are:
// I1, I8, I16, I32, I64
// F64
// STR

// Constants are kept separately for each full length type (I64, F64) and are
// referenced with an ID

// Type I format:   8-bit metadata, 48-bit constant, 8-bit opcode
// [metadata] [constant]        I1_CONST
// [metadata] [constant]        I8_CONST
// [metadata] [constant]        I16_CONST
// [metadata] [constant]        I32_CONST
// [metadata] [I64 constant ID] I64_CONST
// [metadata] [F64 constant ID] F64_CONST

// Type II format:  8-bit metadata, 24-bit ARG0, 24-bit ARG1, 8-bit opcode
// [metadata] [ARG0] [ARG1] I1_CMP
// [metadata] [ARG0] [ARG1] I1_PHI2
// [metadata] [ARG0] [ARG1] I8_ADD
// [metadata] [ARG0] [ARG1] I8_MUL
// [metadata] [ARG0] [ARG1] I8_SUB
// [metadata] [ARG0] [ARG1] I8_DIV
// [metadata] [ARG0] [ARG1] I8_CMP
// [metadata] [ARG0] [ARG1] I8_PHI2
// [metadata] [ARG0] [ARG1] I16_ADD
// [metadata] [ARG0] [ARG1] I16_MUL
// [metadata] [ARG0] [ARG1] I16_SUB
// [metadata] [ARG0] [ARG1] I16_DIV
// [metadata] [ARG0] [ARG1] I16_CMP
// [metadata] [ARG0] [ARG1] I16_PHI2
// [metadata] [ARG0] [ARG1] I32_ADD
// [metadata] [ARG0] [ARG1] I32_MUL
// [metadata] [ARG0] [ARG1] I32_SUB
// [metadata] [ARG0] [ARG1] I32_DIV
// [metadata] [ARG0] [ARG1] I32_CMP
// [metadata] [ARG0] [ARG1] I32_PHI2
// [metadata] [ARG0] [ARG1] I64_ADD
// [metadata] [ARG0] [ARG1] I64_MUL
// [metadata] [ARG0] [ARG1] I64_SUB
// [metadata] [ARG0] [ARG1] I64_DIV
// [metadata] [ARG0] [ARG1] I64_CMP
// [metadata] [ARG0] [ARG1] I64_PHI2
// [metadata] [ARG0] [ARG1] F64_ADD
// [metadata] [ARG0] [ARG1] F64_MUL
// [metadata] [ARG0] [ARG1] F64_SUB
// [metadata] [ARG0] [ARG1] F64_DIV
// [metadata] [ARG0] [ARG1] F64_CMP
// [metadata] [ARG0] [ARG1] F64_PHI2
// [metadata] [ARG0] [ARG1] STORE

// Type III format: 8-bit metadata, 24-bit 0s, 24-bit ARG0, 8-bit opcode
// [metadata] [0] [ARG0] I1_LNOT
// [metadata] [0] [ARG0] I1_ZEXT_I64
// [metadata] [0] [ARG0] I8_ZEXT_I64
// [metadata] [0] [ARG0] I16_ZEXT_I64
// [metadata] [0] [ARG0] I32_ZEXT_I64
// [metadata] [0] [ARG0] I64_CONV_F64
// [metadata] [0] [ARG0] F64_CONV_I64
// [metadata] [0] [ARG0] LOAD

// Type IV format: 8-bit metadata, 24-bit ARG0, 24-bit ARG1, 8-bit opcode,
// 24-bit ARG2, 24 [metadata] [0] []