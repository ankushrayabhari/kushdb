#pragma once

namespace kush::compile {

template <typename ImplTraits>
class Program {
 public:
  virtual ~Program() = default;

  using BasicBlock = typename ImplTraits::BasicBlock;
  using Value = typename ImplTraits::Value;
  using CompType = typename ImplTraits::CompType;

  // Compile
  virtual void Compile() const = 0;
  virtual void Execute() const = 0;

  // Control Flow
  virtual BasicBlock* GenerateBlock() = 0;
  virtual void SetCurrentBlock(BasicBlock* b) = 0;
  virtual void Branch(BasicBlock* b) = 0;
  virtual void Branch(BasicBlock* b, Value* cond) = 0;

  // I32
  virtual Value* AddI32(Value* v1, Value* v2) = 0;
  virtual Value* MulI32(Value* v1, Value* v2) = 0;
  virtual Value* DivI32(Value* v1, Value* v2) = 0;
  virtual Value* SubI32(Value* v1, Value* v2) = 0;

  // I64
  virtual Value* AddI64(Value* v1, Value* v2) = 0;
  virtual Value* MulI64(Value* v1, Value* v2) = 0;
  virtual Value* DivI64(Value* v1, Value* v2) = 0;
  virtual Value* SubI64(Value* v1, Value* v2) = 0;
  virtual Value* CmpI(CompType cmp, Value* v1, Value* v2) = 0;

  // F
  virtual Value* AddF(Value* v1, Value* v2) = 0;
  virtual Value* MulF(Value* v1, Value* v2) = 0;
  virtual Value* DivF(Value* v1, Value* v2) = 0;
  virtual Value* SubF(Value* v1, Value* v2) = 0;
  virtual Value* CmpF(CompType cmp, Value* v1, Value* v2) = 0;
};

}  // namespace kush::compile
