#pragma once

#include "asmjit/x86.h"

#include "compile/khir/program_builder.h"
#include "compile/khir/type_manager.h"

namespace kush::khir {

class ExceptionErrorHandler : public asmjit::ErrorHandler {
 public:
  void handleError(asmjit::Error err, const char* message,
                   asmjit::BaseEmitter* origin) override;
};

class ASMBackend : public Backend {
 public:
  ASMBackend();
  virtual ~ASMBackend() = default;

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

  // Globals
  void TranslateGlobalConstCharArray(std::string_view s) override;
  void TranslateGlobalPointer(
      bool constant, Type t, uint64_t v,
      const std::vector<uint64_t>& i64_constants,
      const std::vector<double>& f64_constants) override;
  void TranslateGlobalStruct(bool constant, Type t,
                             absl::Span<const uint64_t> v,
                             const std::vector<uint64_t>& i64_constants,
                             const std::vector<double>& f64_constants) override;
  void TranslateGlobalArray(bool constant, Type t, absl::Span<const uint64_t> v,
                            const std::vector<uint64_t>& i64_constants,
                            const std::vector<double>& f64_constants) override;

  // Instructions
  void TranslateFuncDecl(bool pub, bool external, std::string_view name,
                         Type function_type) override;
  void TranslateFuncBody(int func_idx,
                         const std::vector<uint64_t>& i64_constants,
                         const std::vector<double>& f64_constants,
                         const std::vector<int>& basic_block_order,
                         const std::vector<std::pair<int, int>>& basic_blocks,
                         const std::vector<uint64_t>& instructions) override;

  // Program
  void Compile() const override;
  void Execute() const override;

 private:
  asmjit::JitRuntime rt_;
  asmjit::CodeHolder code_;
  ExceptionErrorHandler err_handler_;
  std::unique_ptr<asmjit::x86::Assembler> asm_;
  asmjit::Section *text_section_, *data_section_;

  std::vector<asmjit::Label> global_char_arrays_;
  std::vector<asmjit::Label> global_pointers_;
  std::vector<asmjit::Label> global_arrays_;
  std::vector<asmjit::Label> global_structs_;
  mutable std::chrono::time_point<std::chrono::system_clock> start, gen, comp,
      link, end;
};

}  // namespace kush::khir