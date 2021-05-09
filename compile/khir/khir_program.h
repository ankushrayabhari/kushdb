#pragma once

#include <cstdint>
#include <vector>

#include "absl/types/span.h"

#include "type_safe/strong_typedef.hpp"

namespace kush::khir {

struct Type : type_safe::strong_typedef<Type, uint16_t>,
              type_safe::strong_typedef_op::equality_comparison<Type> {
  using strong_typedef::strong_typedef;
};

struct Value : type_safe::strong_typedef<Value, uint32_t>,
               type_safe::strong_typedef_op::equality_comparison<Value> {
  using strong_typedef::strong_typedef;
};

enum class CompType { EQ, NE, LT, LE, GT, GE };

class KHIRProgram {
 public:
  KHIRProgram();

  /*
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

   // Memory
   Value Alloca(Type size);
   Value NullPtr(Type t);
   Value GetElementPtr(Type t, Value ptr, absl::Span<const int32_t> idx);
   Value PointerCast(Value v, Type t);
   void Store(Value ptr, Value v);
   Value Load(Value ptr);

   // Function
   Function CreateFunction(Type result_type, absl::Span<const Type> arg_types);
   Function CreatePublicFunction(Type result_type,
                                 absl::Span<const Type> arg_types,
                                 std::string_view name);
   Function DeclareExternalFunction(std::string_view name, Type result_type,
                                    absl::Span<const Type> arg_types);
   Function GetFunction(std::string_view name);
   std::vector<Value> GetFunctionArguments(Function func);
   void Return(Value v);
   void Return();
   Value Call(Function func, absl::Span<const Value> arguments = {});
   Value Call(Value func, Type type, absl::Span<const Value> arguments = {});

   // Control Flow
   BasicBlock GenerateBlock();
   BasicBlock CurrentBlock();
   bool IsTerminated(BasicBlock b);
   void SetCurrentBlock(BasicBlock b);
   void Branch(BasicBlock b);
   void Branch(Value cond, BasicBlock b1, BasicBlock b2);
   Value Phi(Type type);
   void AddToPhi(Value phi, Value v, BasicBlock b);
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
  std::vector<uint64_t> instructions_;
};

}  // namespace kush::khir