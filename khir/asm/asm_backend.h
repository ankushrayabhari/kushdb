#pragma once

#include <cstdio>

#include "asmjit/x86.h"

#include "khir/opcode.h"
#include "khir/program_builder.h"
#include "khir/type_manager.h"

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
  void Execute() override;

 private:
  uint64_t OutputConstant(uint64_t instr, const TypeManager& type_manager,
                          const std::vector<uint64_t>& i64_constants,
                          const std::vector<double>& f64_constants,
                          const std::vector<std::string>& char_array_constants,
                          const std::vector<StructConstant>& struct_constants,
                          const std::vector<ArrayConstant>& array_constants,
                          const std::vector<Global>& globals);
  void ComparisonInRdx(khir::Opcode op);
  void TranslateInstr(const TypeManager& type_manager,
                      const std::vector<uint64_t>& i64_constants,
                      const std::vector<double>& f64_constants,
                      const std::vector<asmjit::Label>& basic_blocks,
                      const std::vector<Function>& functions,
                      const asmjit::Label& epilogue,
                      std::vector<int64_t>& offsets,
                      const std::vector<uint64_t>& instructions, int instr_idx,
                      int64_t& static_stack_alloc);
  asmjit::JitRuntime rt_;
  asmjit::CodeHolder code_;
  ExceptionErrorHandler err_handler_;
  std::unique_ptr<asmjit::x86::Assembler> asm_;

  std::vector<asmjit::Label> char_array_constants_;
  std::vector<asmjit::Label> globals_;

  std::vector<void*> external_func_addr_;
  std::vector<asmjit::Label> internal_func_labels_;
  asmjit::Label compute_label_;
  int num_floating_point_args_;
  int num_regular_args_;
  int num_stack_args_;

  std::vector<int64_t> regular_call_args_;
  std::vector<int64_t> floating_point_call_args_;

  absl::flat_hash_map<std::string, asmjit::Label> breakpoints_;

  std::chrono::time_point<std::chrono::system_clock> start, comp, end;
};

}  // namespace kush::khir