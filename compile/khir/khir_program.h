#pragma once

#include <cstdint>
#include <vector>

#include "type_safe/strong_typedef.hpp"

namespace kush::khir {

enum class Opcode : int8_t {
  // Comparison
  FCMP_OEQ,
  FCMP_ONE,
  FCMP_OLT,
  FCMP_OLE,
  FCMP_OGT,
  FCMP_OGE,
  ICMP_EQ,
  ICMP_NE,
  ICMP_SGT,
  ICMP_SGE,
  ICMP_SLT,
  ICMP_SLE,

  // I1
  LNOT_I1,
  CONST_I1,
  CMP_I1,

  // I8
  ADD_I8,
  MUL_I8,
  DIV_I8,
  SUB_I8,
  CMP_I8,
  CONST_I8,
};

enum class CompType : int8_t {
  FCMP_OEQ,
  FCMP_ONE,
  FCMP_OLT,
  FCMP_OLE,
  FCMP_OGT,
  FCMP_OGE,
  ICMP_EQ,
  ICMP_NE,
  ICMP_SGT,
  ICMP_SGE,
  ICMP_SLT,
  ICMP_SLE,
};

class KhirProgram {
 public:
  KhirProgram();
  ~KhirProgram() = default;

  // Instruction Offset
  struct Value : type_safe::strong_typedef<Value, int32_t>,
                 type_safe::strong_typedef_op::equality_comparison<Value> {
    using strong_typedef::strong_typedef;
  };

  using CompType = CompType;

  // I1
  Value LNotI1(Value v);
  Value CmpI1(CompType cmp, Value v1, Value v2);
  Value ConstI1(bool v);

  // I8
  Value AddI8(Value v1, Value v2);
  Value MulI8(Value v1, Value v2);
  Value DivI8(Value v1, Value v2);
  Value SubI8(Value v1, Value v2);
  Value CmpI8(CompType cmp, Value v1, Value v2);
  Value ConstI8(int8_t v);

  /*
    // I16
    Value AddI16(Value v1, Value v2);
    Value MulI16(Value v1, Value v2);
    Value DivI16(Value v1, Value v2);
    Value SubI16(Value v1, Value v2);
    Value CmpI16(CompType cmp, Value v1, Value v2);
    Value ConstI16(int16_t v);

    // I32
    Value AddI32(Value v1, Value v2);
    Value MulI32(Value v1, Value v2);
    Value DivI32(Value v1, Value v2);
    Value SubI32(Value v1, Value v2);
    Value CmpI32(CompType cmp, Value v1, Value v2);
    Value ConstI32(int32_t v);

    // I64
    Value AddI64(Value v1, Value v2);
    Value MulI64(Value v1, Value v2);
    Value DivI64(Value v1, Value v2);
    Value SubI64(Value v1, Value v2);
    Value CmpI64(CompType cmp, Value v1, Value v2);
    Value ConstI64(int64_t v);
    Value ZextI64(Value v);
    Value F64ConversionI64(Value v);

    // F64
    Value AddF64(Value v1, Value v2);
    Value MulF64(Value v1, Value v2);
    Value DivF64(Value v1, Value v2);
    Value SubF64(Value v1, Value v2);
    Value CmpF64(CompType cmp, Value v1, Value v2);
    Value ConstF64(double v);
    Value CastSignedIntToF64(Value v);
  */
 private:
  void AppendValue(Value v);
  void AppendOpcode(Opcode opcode);
  void AppendCompType(CompType c);
  void AppendLiteral(bool v);
  void AppendLiteral(int8_t v);
  void AppendLiteral(int16_t v);
  void AppendLiteral(int32_t v);
  void AppendLiteral(int64_t v);
  void AppendLiteral(double v);

  std::vector<int8_t> instructions_;
};

}  // namespace kush::khir