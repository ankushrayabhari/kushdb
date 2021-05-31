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
  ASMBackend() = default;
  virtual ~ASMBackend() = default;

  void Translate(const TypeManager& type_manager,
                 const std::vector<uint64_t>& i64_constants,
                 const std::vector<double>& f64_constants,
                 const std::vector<std::string>& char_array_constants,
                 const std::vector<StructConstant>& struct_constants,
                 const std::vector<ArrayConstant>& array_constants,
                 const std::vector<Global>& globals,
                 const std::vector<Function>& functions) override;

  // Program
  void Compile() const override;
  void Execute() const override;

 private:
  uint64_t OutputConstant(uint64_t instr, const TypeManager& type_manager,
                          const std::vector<uint64_t>& i64_constants,
                          const std::vector<double>& f64_constants,
                          const std::vector<std::string>& char_array_constants,
                          const std::vector<StructConstant>& struct_constants,
                          const std::vector<ArrayConstant>& array_constants,
                          const std::vector<Global>& globals);

  asmjit::JitRuntime rt_;
  asmjit::CodeHolder code_;
  ExceptionErrorHandler err_handler_;
  std::unique_ptr<asmjit::x86::Assembler> asm_;
  asmjit::Section *text_section_, *data_section_;

  std::vector<asmjit::Label> char_array_constants_;
  std::vector<asmjit::Label> globals_;

  mutable std::chrono::time_point<std::chrono::system_clock> start, gen, comp,
      link, end;
};

}  // namespace kush::khir