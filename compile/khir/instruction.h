#pragma once

// Types: VOID, I1, I8, I16, I32, I64, F64, STRING

// Type I format:
//      1-bit marker
//      23-bit 0
//      32-bit max-length, variable-length constant/ID
//      8-bit opcode
// =============================================================================
// [D] [0] [1-bit constant]  I1_CONST
// [D] [0] [8-bit constant]  I8_CONST
// [D] [0] [16-bit constant] I8_CONST
// [D] [0] [32-bit constant] I32_CONST
// [D] [0] [32-bit ID]       I64_CONST
// [D] [0] [32-bit ID]       F64_CONST
// [D] [0] [32-bit ID]       STR_CONST

// Type II format:
//      1-bit marker
//      7-bit METADATA,
//      [24-bit ARG] ** 2
//      8-bit opcode
// =============================================================================
// [D] [0]  [ARG0] [ARG1] I1_CMP
// [D] [0]  [ARG0] [ARG1] I8_ADD
// [D] [0]  [ARG0] [ARG1] I8_MUL
// [D] [0]  [ARG0] [ARG1] I8_SUB
// [D] [0]  [ARG0] [ARG1] I8_DIV
// [D] [0]  [ARG0] [ARG1] I8_CMP
// [D] [0]  [ARG0] [ARG1] I16_ADD
// [D] [0]  [ARG0] [ARG1] I16_MUL
// [D] [0]  [ARG0] [ARG1] I16_SUB
// [D] [0]  [ARG0] [ARG1] I16_DIV
// [D] [0]  [ARG0] [ARG1] I16_CMP
// [D] [0]  [ARG0] [ARG1] I32_ADD
// [D] [0]  [ARG0] [ARG1] I32_MUL
// [D] [0]  [ARG0] [ARG1] I32_SUB
// [D] [0]  [ARG0] [ARG1] I32_DIV
// [D] [0]  [ARG0] [ARG1] I32_CMP
// [D] [0]  [ARG0] [ARG1] I64_ADD
// [D] [0]  [ARG0] [ARG1] I64_MUL
// [D] [0]  [ARG0] [ARG1] I64_SUB
// [D] [0]  [ARG0] [ARG1] I64_DIV
// [D] [0]  [ARG0] [ARG1] I64_CMP
// [D] [0]  [ARG0] [ARG1] F64_ADD
// [D] [0]  [ARG0] [ARG1] F64_MUL
// [D] [0]  [ARG0] [ARG1] F64_SUB
// [D] [0]  [ARG0] [ARG1] F64_DIV
// [D] [0]  [ARG0] [ARG1] F64_CMP
// [D] [0]  [ARG0] [ARG1] STORE
// [D] [0]  [ARG0] [ARG1] CONDBR
// [D] [0]  [0]    [ARG0] I1_LNOT
// [D] [0]  [0]    [ARG0] I1_ZEXT_I64
// [D] [0]  [0]    [ARG0] I8_ZEXT_I64
// [D] [0]  [0]    [ARG0] I16_ZEXT_I64
// [D] [0]  [0]    [ARG0] I32_ZEXT_I64
// [D] [0]  [0]    [ARG0] I64_CONV_F64
// [D] [0]  [0]    [ARG0] F64_CONV_I64
// [D] [0]  [0]    [ARG0] LOAD
// [D] [0]  [0]    [ARG0] RETURN_VALUE
// [D] [0]  [0]    [ARG0] BR
// [D] [0]  [0]    [0]    RETURN
// [D] [MD] [ARG0] [ARG1] PHI_I1
//      - MD: number of PHI_EXT
// [D] [MD] [ARG0] [ARG1] PHI_I8
//      - MD: number of PHI_EXT
// [D] [MD] [ARG0] [ARG1] PHI_I16
//      - MD: number of PHI_EXT
// [D] [MD] [ARG0] [ARG1] PHI_I32
//      - MD: number of PHI_EXT
// [D] [MD] [ARG0] [ARG1] PHI_I64
//      - MD: number of PHI_EXT
// [D] [MD] [ARG0] [ARG1] PHI_F64
//      - MD: number of PHI_EXT
// [D] [MD] [ARG0] [ARG1] PHI_STRING
//      - MD: number of PHI_EXT
// [0] [0]  [ARG0] [ARG1] PHI_EXT
// [0] [0]  [ARG0] [ARG1] CALL_EXT

// Type III format:
//      1-bit marker
//      7-bit METADATA
//      8-bit Type ID
//      [8-bit SARG] ** 2
//      24-bit ARG
//      8-bit opcode
// =============================================================================
// [D] [MD] [ID] [0] [ARG0] CALL
//      - MD: number of CALL_EXT
// [D] [MD] [ID] [SARG0] [SARG1] [ARG0] GEP
//      - MD: number of GEP_EXT

// Type IV Format:
//      8-bit Type ID
//      [8-bit SARG] ** 5
//      8-bit opcode
// =============================================================================
// [ID] [SARG0] [SARG1] [SARG2] [SARG3] [SARG4] [SARG5] GEP_EXT