#pragma once

#include "khir/program.h"
#include "khir/type_manager.h"

namespace kush::khir {

class ProgramPrinter : public TypeTranslator, public ProgramTranslator {
 public:
  virtual ~ProgramPrinter() = default;

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

  void Translate(const Program& program) override;

  void OutputInstr(int idx, const Program& program, const Function& func);

 private:
  const TypeManager* manager_;
  std::vector<std::string> type_to_string_;
};

}  // namespace kush::khir