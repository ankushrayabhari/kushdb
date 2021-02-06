#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace kush::compile {

template <typename Impl>
class ProgramBuilder {
 public:
  virtual ~ProgramBuilder() = default;

  using BasicBlock = typename Impl::BasicBlock;
  using Value = typename Impl::Value;
  using PhiValue = typename Impl::PhiValue;
  using CompType = typename Impl::CompType;
  using Constant = typename Impl::Constant;
  using Function = typename Impl::Function;
  using Type = typename Impl::Type;

  // Types
  virtual Type& I8Type() = 0;
  virtual Type& I16Type() = 0;
  virtual Type& I32Type() = 0;
  virtual Type& I64Type() = 0;
  virtual Type& UI32Type() = 0;
  virtual Type& F64Type() = 0;
  virtual Type& StructType(std::vector<std::reference_wrapper<Type>> types) = 0;
  virtual Type& PointerType(Type& type) = 0;
  virtual Type& ArrayType(Type& type) = 0;

  // Memory
  virtual Value& Malloc(Value& size) = 0;
  virtual void Free(Value& ptr) = 0;
  virtual Value& NullPtr() = 0;
  virtual Value& GetElementPtr(
      Type& t, Value& ptr, std::vector<std::reference_wrapper<Value>> idx) = 0;
  virtual Value& PointerCast(Value& v, Type& t) = 0;
  virtual void Store(Value& ptr, Value& v) = 0;
  virtual Value& Load(Value& ptr) = 0;
  virtual void Memcpy(Value& dest, Value& src, Value& length) = 0;

  // Function
  virtual Function& CurrentFunction() = 0;
  virtual Function& GetFunction(std::string_view name) = 0;
  virtual Function& DeclareFunction(
      std::string_view name, Type& result_type,
      std::vector<std::reference_wrapper<Type>> arg_types) = 0;
  virtual Value& Call(
      std::string_view name,
      std::vector<std::reference_wrapper<Value>> arguments = {}) = 0;

  // Control Flow
  virtual BasicBlock& GenerateBlock() = 0;
  virtual BasicBlock& CurrentBlock() = 0;
  virtual void SetCurrentBlock(BasicBlock& b) = 0;
  virtual void Branch(BasicBlock& b) = 0;
  virtual void Branch(Value& cond, BasicBlock& b1, BasicBlock& b2) = 0;
  virtual PhiValue& Phi(Type& type) = 0;
  virtual void AddToPhi(PhiValue& phi, Value& v, BasicBlock& b) = 0;

  // I8
  virtual Value& AddI8(Value& v1, Value& v2) = 0;
  virtual Value& MulI8(Value& v1, Value& v2) = 0;
  virtual Value& DivI8(Value& v1, Value& v2) = 0;
  virtual Value& SubI8(Value& v1, Value& v2) = 0;
  virtual Value& CmpI8(CompType cmp, Value& v1, Value& v2) = 0;
  virtual Value& LNotI8(Value& v) = 0;
  virtual Value& ConstI8(int8_t v) = 0;

  // I16
  virtual Value& AddI16(Value& v1, Value& v2) = 0;
  virtual Value& MulI16(Value& v1, Value& v2) = 0;
  virtual Value& DivI16(Value& v1, Value& v2) = 0;
  virtual Value& SubI16(Value& v1, Value& v2) = 0;
  virtual Value& CmpI16(CompType cmp, Value& v1, Value& v2) = 0;
  virtual Value& ConstI16(int16_t v) = 0;

  // I32
  virtual Value& AddI32(Value& v1, Value& v2) = 0;
  virtual Value& MulI32(Value& v1, Value& v2) = 0;
  virtual Value& DivI32(Value& v1, Value& v2) = 0;
  virtual Value& SubI32(Value& v1, Value& v2) = 0;
  virtual Value& CmpI32(CompType cmp, Value& v1, Value& v2) = 0;
  virtual Value& ConstI32(int32_t v) = 0;

  // UI32
  virtual Value& AddUI32(Value& v1, Value& v2) = 0;
  virtual Value& MulUI32(Value& v1, Value& v2) = 0;
  virtual Value& DivUI32(Value& v1, Value& v2) = 0;
  virtual Value& SubUI32(Value& v1, Value& v2) = 0;
  virtual Value& CmpUI32(CompType cmp, Value& v1, Value& v2) = 0;
  virtual Value& ConstUI32(uint32_t v) = 0;

  // I64
  virtual Value& AddI64(Value& v1, Value& v2) = 0;
  virtual Value& MulI64(Value& v1, Value& v2) = 0;
  virtual Value& DivI64(Value& v1, Value& v2) = 0;
  virtual Value& SubI64(Value& v1, Value& v2) = 0;
  virtual Value& CmpI64(CompType cmp, Value& v1, Value& v2) = 0;
  virtual Value& ConstI64(int64_t v) = 0;

  // F64
  virtual Value& AddF64(Value& v1, Value& v2) = 0;
  virtual Value& MulF64(Value& v1, Value& v2) = 0;
  virtual Value& DivF64(Value& v1, Value& v2) = 0;
  virtual Value& SubF64(Value& v1, Value& v2) = 0;
  virtual Value& CmpF64(CompType cmp, Value& v1, Value& v2) = 0;
  virtual Value& ConstF64(double v) = 0;
};

}  // namespace kush::compile