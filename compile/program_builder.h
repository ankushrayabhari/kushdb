#pragma once

namespace kush::compile {

template <typename Impl>
class ProgramBuilder {
 public:
  virtual ~ProgramBuilder() = default;

  using BasicBlock = typename Impl::BasicBlock;
  using Value = typename Impl::Value;
  using CompType = typename Impl::CompType;
  using Constant = typename Impl::Constant;

  // Control Flow
  virtual BasicBlock& GenerateBlock() = 0;
  virtual BasicBlock& CurrentBlock() = 0;
  virtual void SetCurrentBlock(BasicBlock& b) = 0;
  virtual void Branch(BasicBlock& b) = 0;
  virtual void Branch(Value& cond, BasicBlock& b1, BasicBlock& b2) = 0;
  virtual Value& Phi(Value& v1, BasicBlock& b1, Value& v2, BasicBlock& b2) = 0;

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
