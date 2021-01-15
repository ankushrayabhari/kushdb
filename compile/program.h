#pragma once

namespace kush::compile {

class Program {
 public:
  // Compile
  virtual ~Program() = default;
  virtual void Compile() const = 0;
  virtual void Execute() const = 0;
};

template <typename ImplTraits>
class ProgramBuilder {
 public:
  virtual ~ProgramBuilder() = default;

  using BasicBlock = typename ImplTraits::BasicBlock;
  using Value = typename ImplTraits::Value;
  using CompType = typename ImplTraits::CompType;

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
  virtual Value* Constant(int32_t v) = 0;

  // I64
  virtual Value* AddI64(Value* v1, Value* v2) = 0;
  virtual Value* MulI64(Value* v1, Value* v2) = 0;
  virtual Value* DivI64(Value* v1, Value* v2) = 0;
  virtual Value* SubI64(Value* v1, Value* v2) = 0;
  virtual Value* Constant(int64_t v) = 0;

  // F
  virtual Value* AddF(Value* v1, Value* v2) = 0;
  virtual Value* MulF(Value* v1, Value* v2) = 0;
  virtual Value* DivF(Value* v1, Value* v2) = 0;
  virtual Value* SubF(Value* v1, Value* v2) = 0;
  virtual Value* Constant(double v) = 0;

  // Comparison
  virtual Value* CmpI(CompType cmp, Value* v1, Value* v2) = 0;
  virtual Value* CmpF(CompType cmp, Value* v1, Value* v2) = 0;
};

}  // namespace kush::compile
