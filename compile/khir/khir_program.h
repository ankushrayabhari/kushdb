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

  uint32_t GetID() { return static_cast<uint32_t>(*this); }

  Value GetAdjacentInstruction(uint32_t offset) {
    return static_cast<Value>(static_cast<uint32_t>(*this) + offset);
  }
};

struct FunctionRef
    : type_safe::strong_typedef<FunctionRef, int>,
      type_safe::strong_typedef_op::equality_comparison<FunctionRef> {
  using strong_typedef::strong_typedef;

  int GetID() { return static_cast<int>(*this); }
};

struct BasicBlockRef
    : type_safe::strong_typedef<BasicBlockRef, std::pair<int, int>>,
      type_safe::strong_typedef_op::equality_comparison<BasicBlockRef> {
  using strong_typedef::strong_typedef;

  int GetFunctionID() { return static_cast<std::pair<int, int>>(*this).first; }

  int GetBasicBlockID() {
    return static_cast<std::pair<int, int>>(*this).second;
  }
};

enum class CompType { EQ, NE, LT, LE, GT, GE };

class KHIRProgram {
 public:
  KHIRProgram();

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

  // Function
  FunctionRef CreateFunction(Type result_type,
                             absl::Span<const Type> arg_types);
  FunctionRef CreatePublicFunction(Type result_type,
                                   absl::Span<const Type> arg_types,
                                   std::string_view name);
  FunctionRef GetFunction(std::string_view name);
  absl::Span<const Value> GetFunctionArguments(FunctionRef func);
  void Return(Value v);
  void Return();

  // Control Flow
  BasicBlockRef GenerateBlock();
  BasicBlockRef CurrentBlock();
  bool IsTerminated(BasicBlockRef b);
  void SetCurrentBlock(BasicBlockRef b);
  void Branch(BasicBlockRef b);
  void Branch(Value cond, BasicBlockRef b1, BasicBlockRef b2);
  Value Phi(Type type, uint8_t num_ext);
  void AddToPhi(Value phi, Value v, BasicBlockRef b);

  /*
   Type TypeOf(Value value);
   Value SizeOf(Type type);

   // Memory
   Value Alloca(Type size);
   Value NullPtr(Type t);
   Value GetElementPtr(Type t, Value ptr, absl::Span<const int32_t> idx);
   Value PointerCast(Value v, Type t);
   void Store(Value ptr, Value v);
   Value Load(Value ptr);

   Function DeclareExternalFunction(std::string_view name, Type result_type,
                                   absl::Span<const Type> arg_types);
   Value Call(Function func, absl::Span<const Value> arguments = {});
   Value Call(Value func, Type type, absl::Span<const Value> arguments = {});
 */

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

    // Globals
    Value GlobalConstString(std::string_view s);
    Value GlobalStruct(bool constant, Type t, absl::Span<const Value> v);
    Value GlobalArray(bool constant, Type t, absl::Span<const Value> v);
    Value GlobalPointer(bool constant, Type t, Value v);
    */
 private:
  class Function {
   public:
    Function(Type function_type, Type result_type,
             absl::Span<const Type> arg_types);
    absl::Span<const Value> GetFunctionArguments() const;

    Value Append(uint64_t instr);
    void Update(Value pos, uint64_t instr);
    uint64_t GetInstruction(Value v);

    int GenerateBasicBlock();
    void SetCurrentBasicBlock(int basic_block_id);
    int GetCurrentBasicBlock();
    bool IsTerminated(int basic_block_id);

   private:
    Type return_type_;
    std::vector<Type> arg_types_;
    std::vector<Value> arg_values_;
    Type function_type_;

    std::vector<std::pair<int, int>> basic_blocks_;
    std::vector<uint64_t> instructions_;

    int current_basic_block_;
  };

  TypeManager type_manager_;
  std::vector<Function> functions_;

  Function& GetCurrentFunction();
  int current_function_;
  absl::flat_hash_map<std::string, FunctionRef> name_to_function_;
};

}  // namespace kush::khir