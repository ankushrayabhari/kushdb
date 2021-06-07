#pragma once

#include <cstdint>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/types/span.h"

#include "type_safe/strong_typedef.hpp"

#include "compile/program.h"
#include "khir/type_manager.h"

namespace kush::khir {

class Value {
 public:
  Value() : idx_(UINT32_MAX) {}

  explicit Value(uint32_t idx) : idx_(idx) {}

  Value(uint32_t idx, bool constant_global) : idx_(idx) {
    if (idx_ > 0x7FFFFF) {
      throw std::runtime_error("Invalid idx");
    }

    if (constant_global) {
      idx_ = idx_ | (1 << 23);
    }
  }

  uint32_t Serialize() const { return idx_; }

  uint32_t GetIdx() const { return idx_ & 0x7FFFFF; }

  bool IsConstantGlobal() const { return (idx_ & (1 << 23)) != 0; }

 private:
  uint32_t idx_;
};

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

class StructConstant {
 public:
  StructConstant(khir::Type type, absl::Span<const Value> fields);
  khir::Type Type() const;
  absl::Span<const Value> Fields() const;

 private:
  khir::Type type_;
  std::vector<Value> fields_;
};

class ArrayConstant {
 public:
  ArrayConstant(khir::Type type, absl::Span<const Value> elements);
  khir::Type Type() const;
  absl::Span<const Value> Elements() const;

 private:
  khir::Type type_;
  std::vector<Value> elements_;
};

class Global {
 public:
  Global(bool constant, bool pub, khir::Type type, Value init);
  bool Constant() const;
  bool Public() const;
  khir::Type Type() const;
  Value InitialValue() const;

 private:
  bool constant_;
  bool public_;
  khir::Type type_;
  Value init_;
};

class ProgramBuilder;

class Function {
 public:
  Function(ProgramBuilder& program_builder, std::string_view name,
           Type function_type, Type result_type,
           absl::Span<const Type> arg_types, bool external, bool p,
           void* func = nullptr);
  absl::Span<const Value> GetFunctionArguments() const;

  void InitBody();
  Value Append(uint64_t instr);
  void Update(Value pos, uint64_t instr);
  uint64_t GetInstruction(Value v);

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
  const std::vector<uint64_t>& Instructions() const;

 private:
  ProgramBuilder& program_builder_;
  std::string name_;
  khir::Type return_type_;
  std::vector<khir::Type> arg_types_;
  std::vector<Value> arg_values_;
  khir::Type function_type_;
  void* func_;

  std::vector<std::pair<int, int>> basic_blocks_;
  std::vector<int> basic_block_order_;
  std::vector<uint64_t> instructions_;

  int current_basic_block_;
  bool external_;
  bool public_;
};

class Backend : public compile::Program {
 public:
  virtual ~Backend() = default;

  virtual void Translate(const TypeManager& manager,
                         const std::vector<uint64_t>& i64_constants,
                         const std::vector<double>& f64_constants,
                         const std::vector<std::string>& char_array_constants,
                         const std::vector<StructConstant>& struct_constants,
                         const std::vector<ArrayConstant>& array_constants,
                         const std::vector<Global>& globals,
                         const std::vector<uint64_t>& constant_instrs,
                         const std::vector<Function>& functions) = 0;
};

class ProgramBuilder {
 public:
  ProgramBuilder() = default;

  // Types
  Type VoidType();
  Type I1Type();
  Type I8Type();
  Type I16Type();
  Type I32Type();
  Type I64Type();
  Type F64Type();
  Type StructType(absl::Span<const Type> types, std::string_view name = "");
  Type GetStructType(std::string_view name);
  Type PointerType(Type type);
  Type ArrayType(Type type, int len = 0);
  Type FunctionType(Type result, absl::Span<const Type> args);
  Type TypeOf(Value value);

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
  Value Call(Value func, Type type, absl::Span<const Value> arguments = {});
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
  Value Alloca(Type size);
  Value NullPtr(Type t);
  Value PointerCast(Value v, Type t);
  void StoreI8(Value ptr, Value v);
  void StoreI16(Value ptr, Value v);
  void StoreI32(Value ptr, Value v);
  void StoreI64(Value ptr, Value v);
  void StoreF64(Value ptr, Value v);
  void StorePtr(Value ptr, Value v);
  Value LoadI8(Value ptr);
  Value LoadI16(Value ptr);
  Value LoadI32(Value ptr);
  Value LoadI64(Value ptr);
  Value LoadF64(Value ptr);
  Value LoadPtr(Value ptr);
  Value SizeOf(Type type);
  Value GetElementPtr(Type t, Value ptr, absl::Span<const int32_t> idx);

  // I1
  Value LNotI1(Value v);
  Value CmpI1(CompType cmp, Value v1, Value v2);
  Value ConstI1(bool v);
  Value I64ZextI1(Value v);
  Value I8ZextI1(Value v);

  // I8
  Value AddI8(Value v1, Value v2);
  Value MulI8(Value v1, Value v2);
  Value DivI8(Value v1, Value v2);
  Value SubI8(Value v1, Value v2);
  Value CmpI8(CompType cmp, Value v1, Value v2);
  Value ConstI8(uint8_t v);
  Value I64ZextI8(Value v);
  Value F64ConvI8(Value v);

  // I16
  Value AddI16(Value v1, Value v2);
  Value MulI16(Value v1, Value v2);
  Value DivI16(Value v1, Value v2);
  Value SubI16(Value v1, Value v2);
  Value CmpI16(CompType cmp, Value v1, Value v2);
  Value ConstI16(uint16_t v);
  Value I64ZextI16(Value v);
  Value F64ConvI16(Value v);

  // I32
  Value AddI32(Value v1, Value v2);
  Value MulI32(Value v1, Value v2);
  Value DivI32(Value v1, Value v2);
  Value SubI32(Value v1, Value v2);
  Value CmpI32(CompType cmp, Value v1, Value v2);
  Value ConstI32(uint32_t v);
  Value I64ZextI32(Value v);
  Value F64ConvI32(Value v);

  // I64
  Value AddI64(Value v1, Value v2);
  Value MulI64(Value v1, Value v2);
  Value DivI64(Value v1, Value v2);
  Value SubI64(Value v1, Value v2);
  Value CmpI64(CompType cmp, Value v1, Value v2);
  Value ConstI64(uint64_t v);
  Value F64ConvI64(Value v);

  // F64
  Value AddF64(Value v1, Value v2);
  Value MulF64(Value v1, Value v2);
  Value DivF64(Value v1, Value v2);
  Value SubF64(Value v1, Value v2);
  Value CmpF64(CompType cmp, Value v1, Value v2);
  Value ConstF64(double v);
  Value I64ConvF64(Value v);

  // Constant/Global Aggregates
  Value GlobalConstCharArray(std::string_view s);
  Value ConstantStruct(Type t, absl::Span<const Value> v);
  Value ConstantArray(Type t, absl::Span<const Value> v);
  Value Global(bool constant, bool pub, Type t, Value v);

  void Translate(Backend& backend);

 private:
  TypeManager type_manager_;

  friend class Function;
  std::vector<Function> functions_;
  Function& GetCurrentFunction();
  int current_function_;
  absl::flat_hash_map<std::string, FunctionRef> name_to_function_;

  Value AppendConstantGlobal(uint64_t);
  uint64_t GetConstantGlobalInstr(Value v);
  std::vector<uint64_t> constant_instrs_;
  std::vector<uint64_t> i64_constants_;
  std::vector<double> f64_constants_;
  std::vector<std::string> char_array_constants_;
  std::vector<StructConstant> struct_constants_;
  std::vector<ArrayConstant> array_constants_;

  std::vector<khir::Global> globals_;
};

}  // namespace kush::khir