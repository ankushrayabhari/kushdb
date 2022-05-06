#pragma once

#include <cstdint>

namespace kush::khir {

enum class ConstantOpcode : uint8_t {
  I1_CONST,
  I8_CONST,
  I16_CONST,
  I32_CONST,
  I64_CONST,
  F64_CONST,
  I32_CONST_VEC4,
  I32_CONST_VEC8,
  GLOBAL_CHAR_ARRAY_CONST,
  STRUCT_CONST,
  ARRAY_CONST,
  NULLPTR,
  GLOBAL_REF,
  FUNC_PTR,
  PTR_CONST,
  PTR_CAST,
};

ConstantOpcode ConstantOpcodeFrom(uint8_t t);
uint8_t ConstantOpcodeTo(ConstantOpcode opcode);

enum class Opcode : uint8_t {
  RETURN,
  I1_CMP_EQ,
  I1_CMP_NE,
  I1_LNOT,
  I1_AND,
  I1_OR,
  I1_ZEXT_I64,
  I1_ZEXT_I8,
  I8_ADD,
  I8_MUL,
  I8_SUB,
  I8_CMP_EQ,
  I8_CMP_NE,
  I8_CMP_LT,
  I8_CMP_LE,
  I8_CMP_GT,
  I8_CMP_GE,
  I8_ZEXT_I64,
  I8_CONV_F64,
  I16_ADD,
  I16_MUL,
  I16_SUB,
  I16_CMP_EQ,
  I16_CMP_NE,
  I16_CMP_LT,
  I16_CMP_LE,
  I16_CMP_GT,
  I16_CMP_GE,
  I16_ZEXT_I64,
  I16_CONV_F64,
  I32_ADD,
  I32_MUL,
  I32_SUB,
  I32_CMP_EQ,
  I32_CMP_EQ_ANY_CONST_VEC4,
  I32_CMP_EQ_ANY_CONST_VEC8,
  I32_CMP_NE,
  I32_CMP_LT,
  I32_CMP_LE,
  I32_CMP_GT,
  I32_CMP_GE,
  I32_ZEXT_I64,
  I32_CONV_F64,
  I64_ADD,
  I64_MUL,
  I64_SUB,
  I64_LSHIFT,
  I64_RSHIFT,
  I64_AND,
  I64_XOR,
  I64_OR,
  I64_TRUNC_I16,
  I64_TRUNC_I32,
  I64_CMP_EQ,
  I64_CMP_NE,
  I64_CMP_LT,
  I64_CMP_LE,
  I64_CMP_GT,
  I64_CMP_GE,
  I64_CONV_F64,
  F64_ADD,
  F64_MUL,
  F64_SUB,
  F64_DIV,
  F64_CMP_EQ,
  F64_CMP_NE,
  F64_CMP_LT,
  F64_CMP_LE,
  F64_CMP_GT,
  F64_CMP_GE,
  F64_CONV_I64,
  I8_STORE,
  I16_STORE,
  I32_STORE,
  I64_STORE,
  F64_STORE,
  PTR_STORE,
  CONDBR,
  I1_LOAD,
  I8_LOAD,
  I16_LOAD,
  I32_LOAD,
  I64_LOAD,
  F64_LOAD,
  PTR_LOAD,
  RETURN_VALUE,
  BR,
  PHI_MEMBER,
  CALL,
  PHI,
  PTR_CAST,
  FUNC_ARG,
  ALLOCA,
  CALL_ARG,
  CALL_INDIRECT,
  PTR_CMP_NULLPTR,
  GEP_STATIC,
  GEP_STATIC_OFFSET,
  GEP_DYNAMIC,
  GEP_DYNAMIC_OFFSET,
};

Opcode OpcodeFrom(uint8_t t);
uint8_t OpcodeTo(Opcode opcode);

}  // namespace kush::khir
