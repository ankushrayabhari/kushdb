#pragma once

#include <cstdint>
#include <vector>

#include "type_safe/strong_typedef.hpp"

namespace kush::khir {

enum class Opcode : int8_t {
  // Control Flow
  BR,
  COND_BR,

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

  // I16
  ADD_I16,
  MUL_I16,
  DIV_I16,
  SUB_I16,
  CMP_I16,
  CONST_I16,

  // I32
  ADD_I32,
  MUL_I32,
  DIV_I32,
  SUB_I32,
  CMP_I32,
  CONST_I32,

  // I64
  ADD_I64,
  MUL_I64,
  DIV_I64,
  SUB_I64,
  CMP_I64,
  ZEXT_I64,
  F64_CONV_I64,
  CONST_I64,

  // F64
  ADD_F64,
  MUL_F64,
  DIV_F64,
  SUB_F64,
  CMP_F64,
  CONST_F64,
  I_CONV_F64,
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

// Instruction Offset
struct Value : type_safe::strong_typedef<Value, int32_t>,
               type_safe::strong_typedef_op::equality_comparison<Value> {
  using strong_typedef::strong_typedef;
};

class BasicBlockImpl {
 public:
  BasicBlockImpl(int32_t current_function);
  void Terminate(int32_t offset);
  void Codegen(int32_t offset);
  int32_t CurrentFunction() const;
  bool operator==(const BasicBlockImpl& other) const;
  bool IsTerminated() const;
  bool IsEmpty() const;

 private:
  int32_t begin_offset_;
  int32_t end_offset_;
  int32_t current_function_;
};

// Function Idx, Basic Block Offset
struct BasicBlock
    : type_safe::strong_typedef<Value, std::pair<int32_t, int32_t>>,
      type_safe::strong_typedef_op::equality_comparison<Value> {
  using strong_typedef::strong_typedef;
};

class KhirProgram {
 public:
  KhirProgram();
  ~KhirProgram() = default;

  // Control Flow
  BasicBlock GenerateBlock();
  BasicBlock CurrentBlock();
  bool IsTerminated(BasicBlock b);
  void SetCurrentBlock(BasicBlock b);
  void Branch(BasicBlock b);
  void Branch(Value cond, BasicBlock b1, BasicBlock b2);

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
  Value ZextI64(Value v);
  Value F64ConversionI64(Value v);
  Value ConstI64(int64_t v);

  // F64
  Value AddF64(Value v1, Value v2);
  Value MulF64(Value v1, Value v2);
  Value DivF64(Value v1, Value v2);
  Value SubF64(Value v1, Value v2);
  Value CmpF64(CompType cmp, Value v1, Value v2);
  Value ConstF64(double v);
  Value CastSignedIntToF64(Value v);

 private:
  void AppendValue(Value v);
  void AppendBasicBlock(BasicBlock b);
  void AppendOpcode(Opcode opcode);
  void AppendCompType(CompType c);
  void AppendLiteral(bool v);
  void AppendLiteral(int8_t v);
  void AppendLiteral(int16_t v);
  void AppendLiteral(int32_t v);
  void AppendLiteral(int64_t v);
  void AppendLiteral(double v);

  std::vector<std::vector<int8_t>> instructions_per_function_;
  std::vector<std::vector<BasicBlockImpl>> basic_blocks_per_function_;
  int32_t current_block_;
  int32_t current_function_;
};

}  // namespace kush::khir