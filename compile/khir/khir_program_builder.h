#pragma once

#include <cstdint>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/types/span.h"

#include "type_safe/strong_typedef.hpp"

#include "compile/khir/type_manager.h"

namespace kush::khir {

struct Value : type_safe::strong_typedef<Value, uint32_t>,
               type_safe::strong_typedef_op::equality_comparison<Value> {
  using strong_typedef::strong_typedef;

  uint32_t GetID() const { return static_cast<uint32_t>(*this); }

  Value GetAdjacentInstruction(uint32_t offset) const {
    return static_cast<Value>(static_cast<uint32_t>(*this) + offset);
  }
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

class KHIRProgramBuilder {
 public:
  KHIRProgramBuilder();

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

  // Function
  FunctionRef CreateFunction(Type result_type,
                             absl::Span<const Type> arg_types);
  FunctionRef CreatePublicFunction(Type result_type,
                                   absl::Span<const Type> arg_types,
                                   std::string_view name);
  FunctionRef DeclareExternalFunction(std::string_view name, Type result_type,
                                      absl::Span<const Type> arg_types);
  FunctionRef GetFunction(std::string_view name);
  absl::Span<const Value> GetFunctionArguments(FunctionRef func);
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
  void Store(Value ptr, Value v);
  Value Load(Value ptr);
  Value SizeOf(Type type);
  Value GetElementPtr(Type t, Value ptr, absl::Span<const int32_t> idx);

  // I1
  Value LNotI1(Value v);
  Value CmpI1(CompType cmp, Value v1, Value v2);
  Value ConstI1(bool v);
  Value ZextI1(Value v);

  // I8
  Value AddI8(Value v1, Value v2);
  Value MulI8(Value v1, Value v2);
  Value DivI8(Value v1, Value v2);
  Value SubI8(Value v1, Value v2);
  Value CmpI8(CompType cmp, Value v1, Value v2);
  Value ConstI8(uint8_t v);
  Value ZextI8(Value v);

  // I16
  Value AddI16(Value v1, Value v2);
  Value MulI16(Value v1, Value v2);
  Value DivI16(Value v1, Value v2);
  Value SubI16(Value v1, Value v2);
  Value CmpI16(CompType cmp, Value v1, Value v2);
  Value ConstI16(uint16_t v);
  Value ZextI16(Value v);

  // I32
  Value AddI32(Value v1, Value v2);
  Value MulI32(Value v1, Value v2);
  Value DivI32(Value v1, Value v2);
  Value SubI32(Value v1, Value v2);
  Value CmpI32(CompType cmp, Value v1, Value v2);
  Value ConstI32(uint32_t v);
  Value ZextI32(Value v);

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

  // Globals
  std::function<Value()> GlobalConstCharArray(std::string_view s);
  std::function<Value()> GlobalStruct(bool constant, Type t,
                                      absl::Span<const Value> v);
  std::function<Value()> GlobalArray(bool constant, Type t,
                                     absl::Span<const Value> v);
  std::function<Value()> GlobalPointer(bool constant, Type t, Value v);

 private:
  class Function {
   public:
    Function(std::string_view name, Type function_type, Type result_type,
             absl::Span<const Type> arg_types, bool external);
    absl::Span<const Value> GetFunctionArguments() const;

    Value Append(uint64_t instr);
    void Update(Value pos, uint64_t instr);
    uint64_t GetInstruction(Value v);

    int GenerateBasicBlock();
    void SetCurrentBasicBlock(int basic_block_id);
    int GetCurrentBasicBlock();
    bool IsTerminated(int basic_block_id);

    Type ReturnType();

   private:
    std::string name_;
    Type return_type_;
    std::vector<Type> arg_types_;
    std::vector<Value> arg_values_;
    Type function_type_;

    std::vector<std::pair<int, int>> basic_blocks_;
    std::vector<uint64_t> instructions_;

    int current_basic_block_;
    bool external_;
  };

  TypeManager type_manager_;
  std::vector<Function> functions_;

  Function& GetCurrentFunction();
  int current_function_;
  absl::flat_hash_map<std::string, FunctionRef> name_to_function_;

  class GlobalArrayImpl {
   public:
    GlobalArrayImpl(bool constant, Type type, absl::Span<const uint64_t> init);
    bool Constant() const;
    Type Type() const;
    absl::Span<const uint64_t> InitialValue() const;

   private:
    bool constant_;
    khir::Type type_;
    std::vector<uint64_t> init;
  };

  class GlobalPointerImpl {
   public:
    GlobalPointerImpl(bool constant, Type type, uint64_t init);
    bool Constant() const;
    Type Type() const;
    uint64_t InitialValue() const;

   private:
    bool constant_;
    khir::Type type_;
    uint64_t init;
  };

  class GlobalStructImpl {
   public:
    GlobalStructImpl(bool constant, Type type, absl::Span<const uint64_t> init);
    bool Constant() const;
    Type Type() const;
    absl::Span<const uint64_t> InitialValue() const;

   private:
    bool constant_;
    khir::Type type_;
    std::vector<uint64_t> init;
  };

  std::vector<uint64_t> i64_constants_;
  std::vector<double> f64_constants_;
  std::vector<std::string> global_char_arrays_;
  std::vector<GlobalPointerImpl> global_pointers_;
  std::vector<GlobalArrayImpl> global_arrays_;
  std::vector<GlobalStructImpl> global_structs_;
};

}  // namespace kush::khir