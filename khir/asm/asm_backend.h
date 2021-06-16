#pragma once

#include <cstdio>

#include "asmjit/x86.h"

#include "compile/program.h"
#include "khir/opcode.h"
#include "khir/program_builder.h"
#include "khir/type_manager.h"

namespace kush::khir {

class ExceptionErrorHandler : public asmjit::ErrorHandler {
 public:
  void handleError(asmjit::Error err, const char* message,
                   asmjit::BaseEmitter* origin) override;
};

class StackSlotAllocator {
 public:
  StackSlotAllocator(int32_t initial_size);
  int32_t AllocateSlot();
  int32_t GetSize();

 private:
  int32_t size_;
};

class Operand {
 public:
 private:
};

enum class ASMInst { MOV, XOR, ADD, SUB };

class ASMBackend : public Backend, public compile::Program {
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
                 const std::vector<uint64_t>& constant_instrs,
                 const std::vector<Function>& functions) override;

  // Program
  void Execute() override;

 private:
  uint64_t OutputConstant(uint64_t instr, const TypeManager& type_manager,
                          const std::vector<uint64_t>& constant_instrs,
                          const std::vector<uint64_t>& i64_constants,
                          const std::vector<double>& f64_constants,
                          const std::vector<std::string>& char_array_constants,
                          const std::vector<StructConstant>& struct_constants,
                          const std::vector<ArrayConstant>& array_constants,
                          const std::vector<Global>& globals);
  bool IsGep(khir::Value v, const std::vector<uint64_t>& instructions);
  std::pair<khir::Value, int32_t> Gep(
      khir::Value v, const std::vector<uint64_t>& instructions,
      const std::vector<uint64_t>& constant_instrs,
      const std::vector<uint64_t>& i64_constants);
  void TranslateInstr(const TypeManager& type_manager,
                      const std::vector<uint64_t>& i64_constants,
                      const std::vector<double>& f64_constants,
                      const std::vector<asmjit::Label>& basic_blocks,
                      const std::vector<Function>& functions,
                      const asmjit::Label& epilogue,
                      std::vector<int64_t>& offsets,
                      const std::vector<uint64_t>& instructions,
                      const std::vector<uint64_t>& constant_instrs,
                      int instr_idx, StackSlotAllocator& stack_allocator,
                      const std::vector<int>& register_assign);
  asmjit::JitRuntime rt_;
  asmjit::CodeHolder code_;
  asmjit::Section* text_section_;
  asmjit::Section* data_section_;

  ExceptionErrorHandler err_handler_;
  std::unique_ptr<asmjit::x86::Assembler> asm_;

  std::vector<asmjit::Label> char_array_constants_;
  std::vector<asmjit::Label> globals_;
  asmjit::Label GetConstantGlobal(uint64_t instr);

  std::vector<void*> external_func_addr_;
  std::vector<asmjit::Label> internal_func_labels_;
  asmjit::Label compute_label_;
  int num_floating_point_args_;
  int num_regular_args_;
  int num_stack_args_;

  std::vector<std::pair<khir::Value, khir::Type>> regular_call_args_;
  std::vector<khir::Value> floating_point_call_args_;

  absl::flat_hash_map<std::string, asmjit::Label> breakpoints_;

  std::chrono::time_point<std::chrono::system_clock> start, comp, end;
};

}  // namespace kush::khir