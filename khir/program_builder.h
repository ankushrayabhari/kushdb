#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/types/span.h"

#include "type_safe/strong_typedef.hpp"

#include "khir/program.h"
#include "khir/type_manager.h"
#include "khir/value.h"

namespace kush::khir {

struct FunctionRef
    : type_safe::strong_typedef<FunctionRef, int>,
      type_safe::strong_typedef_op::equality_comparison<FunctionRef> {
  using strong_typedef::strong_typedef;

  int GetID() const { return static_cast<int>(*this); }
};

struct BasicBlockRef
    : type_safe::strong_typedef<BasicBlockRef, std::pair<int, int>>,
      type_safe::strong_typedef_op::equality_comparison<BasicBlockRef> {
  using strong_typedef::strong_typedef;

  int GetFunctionID() const {
    return static_cast<std::pair<int, int>>(*this).first;
  }

  int GetBasicBlockID() const {
    return static_cast<std::pair<int, int>>(*this).second;
  }
};

enum class CompType { EQ, NE, LT, LE, GT, GE };

class ProgramBuilder;

class FunctionBuilder {
 public:
  FunctionBuilder(ProgramBuilder& program_builder, std::string_view name,
                  Type function_type, Type result_type,
                  absl::Span<const Type> arg_types, bool external, bool p,
                  void* func = nullptr);
  absl::Span<const Value> GetFunctionArguments() const;

  void InitBody();
  Value Append(uint64_t instr);
  void Update(Value pos, uint64_t instr);
  uint64_t GetInstruction(Value v) const;

  int GenerateBasicBlock();
  void SetCurrentBasicBlock(int basic_block_id);
  int GetCurrentBasicBlock();
  bool IsTerminated(int basic_block_id);

  khir::Type ReturnType() const;
  khir::Type Type() const;
  bool External() const;
  bool Public() const;
  void* Addr() const;
  std::string_view Name() const;
  const std::vector<int>& BasicBlockOrder() const;
  const std::vector<std::pair<int, int>>& BasicBlocks() const;
  const std::vector<std::vector<int>>& BasicBlockSuccessors() const;
  const std::vector<std::vector<int>>& BasicBlockPredecessors() const;
  const std::vector<uint64_t>& Instructions() const;

 private:
  friend class ProgramBuilder;

  ProgramBuilder& program_builder_;
  std::string name_;
  khir::Type return_type_;
  std::vector<khir::Type> arg_types_;
  std::vector<Value> arg_values_;
  khir::Type function_type_;
  void* func_;

  std::vector<std::pair<int, int>> basic_blocks_;
  std::vector<std::vector<int>> basic_block_successors_;
  std::vector<std::vector<int>> basic_block_predecessors_;
  std::vector<int> basic_block_order_;
  std::vector<uint64_t> instructions_;

  int current_basic_block_;
  bool external_;
  bool public_;
};

class ProgramBuilder {
 public:
  ProgramBuilder() = default;

  // Types
  Type OpaqueType(std::string_view name);
  Type VoidType();
  Type I1Type();
  Type I8Type();
  Type I16Type();
  Type I32Type();
  Type I64Type();
  Type F64Type();
  Type StructType(absl::Span<const Type> types, std::string_view name = "");
  Type GetStructType(std::string_view name);
  Type GetOpaqueType(std::string_view name);
  Type PointerType(Type type);
  Type ArrayType(Type type, int len = 0);
  Type FunctionType(Type result, absl::Span<const Type> args);
  Type TypeOf(Value value) const;
  Value SizeOf(Type type);

  // Function
  FunctionRef CreateFunction(Type result_type,
                             absl::Span<const Type> arg_types);
  FunctionRef CreatePublicFunction(Type result_type,
                                   absl::Span<const Type> arg_types,
                                   std::string_view name);
  FunctionRef DeclareExternalFunction(std::string_view name, Type result_type,
                                      absl::Span<const Type> arg_types,
                                      void* func_ptr);
  FunctionRef GetFunction(std::string_view name);
  absl::Span<const Value> GetFunctionArguments(FunctionRef func);
  Value GetFunctionPointer(FunctionRef func);
  Value Call(FunctionRef func, absl::Span<const Value> arguments = {});
  Value Call(Value func, absl::Span<const Value> arguments = {});
  void Return(Value v);
  void Return();

  // Control Flow
  BasicBlockRef GenerateBlock();
  BasicBlockRef CurrentBlock();
  bool IsTerminated(BasicBlockRef b);
  void SetCurrentBlock(BasicBlockRef b);
  void Branch(BasicBlockRef b);
  void Branch(Value cond, BasicBlockRef b1, BasicBlockRef b2);
  Value Phi(Type type);
  Value PhiMember(Value v);
  void UpdatePhiMember(Value phi, Value phi_member);

  // Memory
  Value ConstPtr(void* t);
  Value NullPtr(Type t);
  Value PointerCast(Value v, Type t);
  Value Alloca(Type t, int num_values = 1);
  Value StaticGEP(Type t, Value ptr, absl::Span<const int32_t> idx);
  Value DynamicGEP(Type t, Value ptr, Value dynamic_idx,
                   absl::Span<const int32_t> idx);
  Value LoadPtr(Value ptr);
  void StorePtr(Value ptr, Value v);
  Value IsNullPtr(Value v);

  // I1
  Value ConstI1(bool v);
  Value LoadI1(Value ptr);
  Value LNotI1(Value v);
  Value AndI1(Value v1, Value v2);
  Value OrI1(Value v1, Value v2);
  Value CmpI1(CompType cmp, Value v1, Value v2);
  Value I64ZextI1(Value v);
  Value I8ZextI1(Value v);

  // I8
  Value ConstI8(uint8_t v);
  Value AddI8(Value v1, Value v2);
  Value MulI8(Value v1, Value v2);
  Value SubI8(Value v1, Value v2);
  Value CmpI8(CompType cmp, Value v1, Value v2);
  Value I64ZextI8(Value v);
  Value F64ConvI8(Value v);
  Value LoadI8(Value ptr);
  void StoreI8(Value ptr, Value v);

  // I16
  Value ConstI16(uint16_t v);
  Value AddI16(Value v1, Value v2);
  Value MulI16(Value v1, Value v2);
  Value SubI16(Value v1, Value v2);
  Value CmpI16(CompType cmp, Value v1, Value v2);
  Value I64ZextI16(Value v);
  Value F64ConvI16(Value v);
  Value LoadI16(Value ptr);
  void StoreI16(Value ptr, Value v);

  // I32
  Value ConstI32(uint32_t v);
  Value AddI32(Value v1, Value v2);
  Value MulI32(Value v1, Value v2);
  Value SubI32(Value v1, Value v2);
  Value CmpI32(CompType cmp, Value v1, Value v2);
  Value I64ZextI32(Value v);
  Value F64ConvI32(Value v);
  Value LoadI32(Value ptr);
  void StoreI32(Value ptr, Value v);

  // I64
  Value ConstI64(uint64_t v);
  Value AddI64(Value v1, Value v2);
  Value MulI64(Value v1, Value v2);
  Value SubI64(Value v1, Value v2);
  Value CmpI64(CompType cmp, Value v1, Value v2);
  Value F64ConvI64(Value v);
  Value LShiftI64(Value v1, uint8_t v2);
  Value RShiftI64(Value v1, uint8_t v2);
  Value AndI64(Value v1, Value v2);
  Value XorI64(Value v1, Value v2);
  Value OrI64(Value v1, Value v2);
  Value I16TruncI64(Value v);
  Value I32TruncI64(Value v);
  Value LoadI64(Value ptr);
  void StoreI64(Value ptr, Value v);

  // F64
  Value ConstF64(double v);
  Value AddF64(Value v1, Value v2);
  Value MulF64(Value v1, Value v2);
  Value DivF64(Value v1, Value v2);
  Value SubF64(Value v1, Value v2);
  Value CmpF64(CompType cmp, Value v1, Value v2);
  Value I64ConvF64(Value v);
  Value LoadF64(Value ptr);
  void StoreF64(Value ptr, Value v);

  // Constant/Global Aggregates
  Value GlobalConstCharArray(std::string_view s);
  Value ConstantStruct(Type t, absl::Span<const Value> v);
  Value ConstantArray(Type t, absl::Span<const Value> v);
  Value Global(Type t, Value v);

  const TypeManager& GetTypeManager() const;

  Program Build();

 private:
  TypeManager type_manager_;

  friend class FunctionBuilder;
  std::vector<FunctionBuilder> functions_;
  FunctionBuilder& GetCurrentFunction();
  const FunctionBuilder& GetCurrentFunction() const;
  int current_function_;
  absl::flat_hash_map<std::string, FunctionRef> name_to_function_;

  Value AppendConstantGlobal(uint64_t);
  uint64_t GetConstantGlobalInstr(Value v) const;
  std::vector<uint64_t> constant_instrs_;
  std::vector<void*> ptr_constants_;
  std::vector<uint64_t> i64_constants_;
  std::vector<double> f64_constants_;
  std::vector<std::string> char_array_constants_;
  std::vector<StructConstant> struct_constants_;
  std::vector<ArrayConstant> array_constants_;

  std::vector<khir::Global> globals_;

  std::unordered_map<bool, Value> i1_const_to_value_;
  std::unordered_map<uint8_t, Value> i8_const_to_value_;
  std::unordered_map<uint16_t, Value> i16_const_to_value_;
  std::unordered_map<uint32_t, Value> i32_const_to_value_;
  std::unordered_map<uint64_t, Value> i64_const_to_value_;
  std::unordered_map<double, Value> f64_const_to_value_;
  std::unordered_map<void*, Value> ptr_const_to_value_;
  absl::flat_hash_map<std::string, Value> string_const_to_value_;
};

}  // namespace kush::khir