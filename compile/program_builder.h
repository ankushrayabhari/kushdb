#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "absl/types/span.h"

namespace kush::compile {

template <typename Impl>
class ProgramBuilder {
 public:
  virtual ~ProgramBuilder() = default;

  using BasicBlock = typename Impl::BasicBlock;
  using Value = typename Impl::Value;
  using CompType = typename Impl::CompType;
  using Function = typename Impl::Function;
  using Type = typename Impl::Type;

  // Types
  virtual Type VoidType() = 0;
  virtual Type I1Type() = 0;
  virtual Type I8Type() = 0;
  virtual Type I16Type() = 0;
  virtual Type I32Type() = 0;
  virtual Type I64Type() = 0;
  virtual Type F64Type() = 0;
  virtual Type StructType(absl::Span<const Type> types,
                          std::string_view name = "") = 0;
  virtual Type GetStructType(std::string_view name) = 0;
  virtual Type PointerType(Type type) = 0;
  virtual Type ArrayType(Type type, int len = 0) = 0;
  virtual Type FunctionType(Type result, absl::Span<const Type> args) = 0;
  virtual Type TypeOf(Value value) = 0;
  virtual Value SizeOf(Type type) = 0;

  // Memory
  virtual Value Alloca(Type size) = 0;
  virtual Value NullPtr(Type t) = 0;
  virtual Value GetElementPtr(Type t, Value ptr,
                              absl::Span<const Value> idx) = 0;
  virtual Value PointerCast(Value v, Type t) = 0;
  virtual void Store(Value ptr, Value v) = 0;
  virtual Value Load(Value ptr) = 0;
  virtual void Memcpy(Value dest, Value src, Value length) = 0;

  // Function
  virtual Function CreateFunction(Type result_type,
                                  absl::Span<const Type> arg_types) = 0;
  virtual Function CreatePublicFunction(Type result_type,
                                        absl::Span<const Type> arg_types,
                                        std::string_view name) = 0;
  virtual Function DeclareExternalFunction(
      std::string_view name, Type result_type,
      absl::Span<const Type> arg_types) = 0;
  virtual Function GetFunction(std::string_view name) = 0;
  virtual std::vector<Value> GetFunctionArguments(Function func) = 0;
  virtual void Return(Value v) = 0;
  virtual void Return() = 0;
  virtual Value Call(Function func, absl::Span<const Value> arguments = {}) = 0;
  virtual Value Call(Value func, Type type,
                     absl::Span<const Value> arguments = {}) = 0;

  // Control Flow
  virtual BasicBlock GenerateBlock() = 0;
  virtual BasicBlock CurrentBlock() = 0;
  virtual bool IsTerminated(BasicBlock b) = 0;
  virtual void SetCurrentBlock(BasicBlock b) = 0;
  virtual void Branch(BasicBlock b) = 0;
  virtual void Branch(Value cond, BasicBlock b1, BasicBlock b2) = 0;
  virtual Value Phi(Type type) = 0;
  virtual void AddToPhi(Value phi, Value v, BasicBlock b) = 0;

  // I1
  virtual Value LNotI1(Value v) = 0;
  virtual Value CmpI1(CompType cmp, Value v1, Value v2) = 0;
  virtual Value ConstI1(bool v) = 0;

  // I8
  virtual Value AddI8(Value v1, Value v2) = 0;
  virtual Value MulI8(Value v1, Value v2) = 0;
  virtual Value DivI8(Value v1, Value v2) = 0;
  virtual Value SubI8(Value v1, Value v2) = 0;
  virtual Value CmpI8(CompType cmp, Value v1, Value v2) = 0;
  virtual Value ConstI8(int8_t v) = 0;

  // I16
  virtual Value AddI16(Value v1, Value v2) = 0;
  virtual Value MulI16(Value v1, Value v2) = 0;
  virtual Value DivI16(Value v1, Value v2) = 0;
  virtual Value SubI16(Value v1, Value v2) = 0;
  virtual Value CmpI16(CompType cmp, Value v1, Value v2) = 0;
  virtual Value ConstI16(int16_t v) = 0;

  // I32
  virtual Value AddI32(Value v1, Value v2) = 0;
  virtual Value MulI32(Value v1, Value v2) = 0;
  virtual Value DivI32(Value v1, Value v2) = 0;
  virtual Value SubI32(Value v1, Value v2) = 0;
  virtual Value CmpI32(CompType cmp, Value v1, Value v2) = 0;
  virtual Value ConstI32(int32_t v) = 0;

  // I64
  virtual Value AddI64(Value v1, Value v2) = 0;
  virtual Value MulI64(Value v1, Value v2) = 0;
  virtual Value DivI64(Value v1, Value v2) = 0;
  virtual Value SubI64(Value v1, Value v2) = 0;
  virtual Value CmpI64(CompType cmp, Value v1, Value v2) = 0;
  virtual Value ConstI64(int64_t v) = 0;
  virtual Value ZextI64(Value v) = 0;
  virtual Value F64ConversionI64(Value v) = 0;

  // F64
  virtual Value AddF64(Value v1, Value v2) = 0;
  virtual Value MulF64(Value v1, Value v2) = 0;
  virtual Value DivF64(Value v1, Value v2) = 0;
  virtual Value SubF64(Value v1, Value v2) = 0;
  virtual Value CmpF64(CompType cmp, Value v1, Value v2) = 0;
  virtual Value ConstF64(double v) = 0;
  virtual Value CastSignedIntToF64(Value v) = 0;

  // Globals
  virtual Value GlobalConstString(std::string_view s) = 0;
  virtual Value GlobalStruct(bool constant, Type t,
                             absl::Span<const Value> v) = 0;
  virtual Value GlobalArray(bool constant, Type t,
                            absl::Span<const Value> v) = 0;
  virtual Value GlobalPointer(bool constant, Type t, Value v) = 0;
};

}  // namespace kush::compile