#pragma once

#include <cstdint>
#include <vector>

class BasicBlockView {
 public:
  virtual uint64_t* begin() const;
  virtual uint64_t* end() const;
};

class FunctionView {
 public:
  virtual bool IsExternal() const;
  virtual BasicBlockView* begin() const;
  virtual BasicBlockView* end() const;
};

class KHIRProgramTranslator {
 public:
  virtual void TranslateFunctionDecl(FunctionView func_decl) = 0;
  virtual void TranslateFunctionBody(FunctionView func) = 0;
  virtual void TranslateBasicBlock(BasicBlockView bb) = 0;
};

class LLVMKHIRProgramTranslator : public KHIRProgramTranslator {
 public:
  void TranslateFunctionDecl(FunctionView func) {
    // output declaration of function
  }

  void TranslateFunctionBody(FunctionView func) override {
    if (func.IsExternal()) {
      return;
    }

    // translate each bb in func
    for (const auto& bb : func) {
      TranslateBasicBlock(bb);
    }
  }

  void TranslateBasicBlock(BasicBlockView bb) override {
    for (auto instr : bb) {
      // translate instr
    }
  }
};