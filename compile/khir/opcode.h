#pragma once

#include <cstdint>

namespace kush::khir {

enum class Opcode : uint8_t {
  I1_CONST,
  I8_CONST,
  I16_CONST,
  I32_CONST,
  I64_CONST,
  F64_CONST,
  STR_CONST,
  RETURN,
  I1_CMP,
  I1_LNOT,
  I1_ZEXT_I64,
  I8_ADD,
  I8_MUL,
  I8_SUB,
  I8_DIV,
  I8_CMP,
  I8_ZEXT_I64,
  I16_ADD,
  I16_MUL,
  I16_SUB,
  I16_DIV,
  I16_CMP,
  I16_ZEXT_I64,
  I32_ADD,
  I32_MUL,
  I32_SUB,
  I32_DIV,
  I32_CMP,
  I32_ZEXT_I64,
  I64_ADD,
  I64_MUL,
  I64_SUB,
  I64_DIV,
  I64_CMP,
  I64_CONV_F64,
  F64_ADD,
  F64_MUL,
  F64_SUB,
  F64_DIV,
  F64_CMP,
  F64_CONV_I64,
  STORE,
  CONDBR,
  LOAD,
  RETURN_VALUE,
  BR,
  PHI_EXT,
  CALL_EXT,
  CALL,
  PHI,
  PTR_CAST,
  GEP,
  FUNC_ARG
};

}  // namespace kush::khir
