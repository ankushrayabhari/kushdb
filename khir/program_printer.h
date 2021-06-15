#pragma once

#include "khir/program_builder.h"
#include "khir/type_manager.h"

namespace kush::khir {

class ProgramPrinter : public Backend, public TypeTranslator {
 public:
  virtual ~ProgramPrinter() = default;

  // Types
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

  void Translate(const TypeManager& manager,
                 const std::vector<uint64_t>& i64_constants,
                 const std::vector<double>& f64_constants,
                 const std::vector<std::string>& char_array_constants,
                 const std::vector<StructConstant>& struct_constants,
                 const std::vector<ArrayConstant>& array_constants,
                 const std::vector<Global>& globals,
                 const std::vector<uint64_t>& constant_instrs,
                 const std::vector<Function>& functions) override;

 private:
  void OutputInstr(int idx, const std::vector<uint64_t>& i64_constants,
                   const std::vector<double>& f64_constants,
                   const std::vector<std::string>& char_array_constants,
                   const std::vector<StructConstant>& struct_constants,
                   const std::vector<ArrayConstant>& array_constants,
                   const std::vector<Global>& globals,
                   const std::vector<uint64_t>& constant_instrs,
                   const std::vector<Function>& functions,
                   const Function& func);

  const TypeManager* manager_;
  std::vector<std::string> type_to_string_;
};

}  // namespace kush::khir