#pragma once

#include <cstdint>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/inlined_vector.h"
#include "absl/types/span.h"

#include "type_safe/strong_typedef.hpp"

namespace kush::khir {

enum class Opcode : int8_t {
  // Types
  SIZEOF,

  // Control Flow
  BR,
  COND_BR,
  PHI,

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

enum class TypeID : int8_t {
  VOID,
  I1,
  I8,
  I16,
  I32,
  I64,
  F64,
  POINTER,
  STRUCT,
  ARRAY,
  FUNCTION
};

// Format for types
// For I1-I64, F64, Void value is type ID
// For Pointers, value is POINTER and then type ID of element
// For Struct, value is STRUCT and then number of fields and then type ID of
// each of the elements
// For ARRAY, value is number of elements and then values
// For FUNCTION, value is return type and then number of args and then arg type
// IDs
struct Type : type_safe::strong_typedef<Type, absl::InlinedVector<int8_t, 4>>,
              type_safe::strong_typedef_op::equality_comparison<Type> {
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
      type_safe::strong_typedef_op::equality_comparison<BasicBlock> {
  using strong_typedef::strong_typedef;
};

class KhirProgram {
 public:
  KhirProgram();
  ~KhirProgram() = default;

  // Types
  Type VoidType();
  Type I1Type();
  Type I8Type();
  Type I16Type();
  Type I32Type();
  Type I64Type();
  Type F64Type();
  Type StructType(absl::Span<const Type> types, std::string_view name);
  Type GetStructType(std::string_view name);
  Type PointerType(Type type);
  Type ArrayType(Type type, int len);
  Type FunctionType(Type result, absl::Span<const Type> args);
  Type TypeOf(Value value);
  Value SizeOf(Type type);

  // Control Flow
  BasicBlock GenerateBlock();
  BasicBlock CurrentBlock();
  bool IsTerminated(BasicBlock b);
  void SetCurrentBlock(BasicBlock b);
  void Branch(BasicBlock b);
  void Branch(Value cond, BasicBlock b1, BasicBlock b2);
  Value Phi(Type type);
  void AddToPhi(Value phi, Value v, BasicBlock b);

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
  void AppendBasicBlockIdx(int32_t b);
  void AppendOpcode(Opcode opcode);
  void AppendCompType(CompType c);
  void AppendType(const Type& t);
  void AppendLiteral(bool v);
  void AppendLiteral(int8_t v);
  void AppendLiteral(int16_t v);
  void AppendLiteral(int32_t v);
  void AppendLiteral(int64_t v);
  void AppendLiteral(double v);

  Value GetValue(int32_t offset);
  int32_t GetBasicBlockIdx(int32_t offset);
  Opcode GetOpcode(int32_t offset);
  Type GetType(int32_t offset);
  CompType GetCompType(int32_t offset);
  bool GetBoolLiteral(int32_t offset);
  int8_t GetI8Literal(int32_t offset);
  int16_t GetI16Literal(int32_t offset);
  int32_t GetI32Literal(int32_t offset);
  int64_t GetI64Literal(int32_t offset);
  double GetF64Literal(int32_t offset);

  std::vector<std::vector<int8_t>> instructions_per_function_;
  std::vector<std::vector<BasicBlockImpl>> basic_blocks_per_function_;
  // Stores a block idx, value pair that contains the phi node for the
  // predecessor
  std::vector<std::vector<std::pair<int32_t, Value>>> phi_values_;
  int32_t current_block_;
  int32_t current_function_;

  absl::flat_hash_map<std::string, Type> named_structs_;
};

}  // namespace kush::khir