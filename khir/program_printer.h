#pragma once

#include "khir/program.h"
#include "khir/type_manager.h"

namespace kush::khir {

class ProgramPrinter : public TypeTranslator {
 public:
  ProgramPrinter(const Program& program);
  virtual ~ProgramPrinter() = default;

  void Print();

  // Types
  void TranslateOpaqueType(std::string_view name) override;
  void TranslateVoidType() override;
  void TranslateI1Type() override;
  void TranslateI8Type() override;
  void TranslateI16Type() override;
  void TranslateI32Type() override;
  void TranslateI64Type() override;
  void TranslateF64Type() override;
  void TranslatePointerType(Type elem) override;
  void TranslateArrayType(Type elem, int len) override;
  void TranslateFunctionType(Type result,
                             absl::Span<const Type> arg_types) override;
  void TranslateStructType(absl::Span<const Type> elem_types) override;

 private:
  void OutputValue(khir::Value v);
  void OutputInstr(int idx, const Function& func);

 private:
  const Program& program_;
  std::vector<std::string> type_to_string_;
};

}  // namespace kush::khir