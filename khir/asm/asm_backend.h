#pragma once

#include <cstdio>

#include "asmjit/x86.h"

#include "khir/asm/reg_alloc_impl.h"
#include "khir/asm/register.h"
#include "khir/asm/register_assignment.h"
#include "khir/backend.h"
#include "khir/opcode.h"
#include "khir/program.h"
#include "khir/type_manager.h"

namespace kush::khir {

class ExceptionErrorHandler : public asmjit::ErrorHandler {
 public:
  void handleError(asmjit::Error err, const char* message,
                   asmjit::BaseEmitter* origin) override;
};

class StackSlotAllocator {
 public:
  StackSlotAllocator();
  int32_t AllocateSlot(int32_t bytes = 8);
  int32_t AllocateSlot(int32_t bytes, int32_t align);
  int32_t GetSize();

 private:
  int32_t size_;
};

struct GEPDynamicInfo {
  Value ptr;
  Value index;
  int32_t offset;
  int32_t type_size;
};

struct GEPStaticInfo {
  Value ptr;
  int32_t offset;
};

class ASMBackend : public Backend {
 public:
  ASMBackend(const Program& program, RegAllocImpl impl);
  virtual ~ASMBackend() = default;

  // Backend
  void Compile();
  void* GetFunction(std::string_view name) override;

  static double CompilationTime();
  static void ResetCompilationTime();

 private:
  void Translate();
  bool IsNullPtr(Value v);
  bool IsConstantPtr(Value v);
  bool IsConstantCastedPtr(Value v);
  Value GetConstantCastedPtr(Value v);
  uint8_t GetByteConstant(Value v);
  uint16_t GetWordConstant(Value v);
  uint32_t GetDwordConstant(Value v);
  uint64_t GetQwordConstant(Value v);
  uint64_t GetPtrConstant(Value v);
  double GetF64Constant(Value v);
  asmjit::Label GetGlobalPointer(Value v);
  GEPStaticInfo StaticGEP(Value v, const std::vector<uint64_t>& instrs);
  GEPDynamicInfo DynamicGEP(Value v, const std::vector<uint64_t>& instrs);

  uint64_t OutputConstant(uint64_t instr);

  void TranslateInstr(int instr_idx, const std::vector<uint64_t>& instructions,
                      const std::vector<asmjit::Label>& basic_blocks,
                      const asmjit::Label& epilogue,
                      std::vector<int32_t>& offsets,
                      StackSlotAllocator& stack_allocator,
                      const std::vector<RegisterAssignment>& register_assign,
                      const std::vector<bool>& gep_materialize, int next_bb);

  asmjit::Label EmbedI8(int8_t d);
  asmjit::Label EmbedF64(double d);
  asmjit::Label EmbedI64(int64_t d);

  asmjit::x86::Mem GetDynamicGEPPtrValue(
      GEPDynamicInfo info, int32_t size, std::vector<int32_t>& offsets,
      const std::vector<RegisterAssignment>& register_assign);
  asmjit::x86::Mem GetStaticGEPPtrValue(
      GEPStaticInfo info, int32_t size, std::vector<int32_t>& offsets,
      const std::vector<RegisterAssignment>& register_assign);

  template <typename T>
  void MoveByteValue(T dest, Value v, std::vector<int32_t>& offsets,
                     const std::vector<RegisterAssignment>& register_assign,
                     int32_t dynamic_stack_alloc = 0);
  template <typename T>
  void AndByteValue(T dest, Value v, std::vector<int32_t>& offsets,
                    const std::vector<RegisterAssignment>& register_assign);
  template <typename T>
  void OrByteValue(T dest, Value v, std::vector<int32_t>& offsets,
                   const std::vector<RegisterAssignment>& register_assign);
  template <typename T>
  void AddByteValue(T dest, Value v, std::vector<int32_t>& offsets,
                    const std::vector<RegisterAssignment>& register_assign);
  template <typename T>
  void SubByteValue(T dest, Value v, std::vector<int32_t>& offsets,
                    const std::vector<RegisterAssignment>& register_assign);
  void MulByteValue(Value v, std::vector<int32_t>& offsets,
                    const std::vector<RegisterAssignment>& register_assign);
  asmjit::x86::GpbLo GetByteValue(
      Value v, std::vector<int32_t>& offsets,
      const std::vector<RegisterAssignment>& register_assign);
  void CmpByteValue(asmjit::x86::GpbLo src, Value v,
                    std::vector<int32_t>& offsets,
                    const std::vector<RegisterAssignment>& register_assign);
  void ZextByteValue(asmjit::x86::Gpq dest, Value v,
                     std::vector<int32_t>& offsets,
                     const std::vector<RegisterAssignment>& register_assign);
  void SextByteValue(asmjit::x86::Gpq dest, Value v,
                     std::vector<int32_t>& offsets,
                     const std::vector<RegisterAssignment>& register_assign);
  asmjit::x86::Mem GetBytePtrValue(
      Value v, std::vector<int32_t>& offsets,
      const std::vector<uint64_t>& instrs,
      const std::vector<RegisterAssignment>& register_assign);

  template <typename T>
  void MoveWordValue(T dest, Value v, std::vector<int32_t>& offsets,
                     const std::vector<RegisterAssignment>& register_assign,
                     int32_t dynamic_stack_alloc = 0);
  template <typename T>
  void AddWordValue(T dest, Value v, std::vector<int32_t>& offsets,
                    const std::vector<RegisterAssignment>& register_assign);
  template <typename T>
  void SubWordValue(T dest, Value v, std::vector<int32_t>& offsets,
                    const std::vector<RegisterAssignment>& register_assign);
  void MulWordValue(asmjit::x86::Gpw dest, Value v,
                    std::vector<int32_t>& offsets,
                    const std::vector<RegisterAssignment>& register_assign);
  asmjit::x86::Gpw GetWordValue(
      Value v, std::vector<int32_t>& offsets,
      const std::vector<RegisterAssignment>& register_assign);
  void CmpWordValue(asmjit::x86::Gpw src, Value v,
                    std::vector<int32_t>& offsets,
                    const std::vector<RegisterAssignment>& register_assign);
  void ZextWordValue(asmjit::x86::Gpq dest, Value v,
                     std::vector<int32_t>& offsets,
                     const std::vector<RegisterAssignment>& register_assign);
  void SextWordValue(asmjit::x86::Gpq dest, Value v,
                     std::vector<int32_t>& offsets,
                     const std::vector<RegisterAssignment>& register_assign);
  asmjit::x86::Mem GetWordPtrValue(
      Value v, std::vector<int32_t>& offsets,
      const std::vector<uint64_t>& instrs,
      const std::vector<RegisterAssignment>& register_assign);

  template <typename T>
  void MoveDWordValue(T dest, Value v, std::vector<int32_t>& offsets,
                      const std::vector<RegisterAssignment>& register_assign,
                      int32_t dynamic_stack_alloc = 0);
  template <typename T>
  void AddDWordValue(T dest, Value v, std::vector<int32_t>& offsets,
                     const std::vector<RegisterAssignment>& register_assign);
  template <typename T>
  void SubDWordValue(T dest, Value v, std::vector<int32_t>& offsets,
                     const std::vector<RegisterAssignment>& register_assign);
  void MulDWordValue(asmjit::x86::Gpd dest, Value v,
                     std::vector<int32_t>& offsets,
                     const std::vector<RegisterAssignment>& register_assign);
  asmjit::x86::Gpd GetDWordValue(
      Value v, std::vector<int32_t>& offsets,
      const std::vector<RegisterAssignment>& register_assign);
  void CmpDWordValue(asmjit::x86::Gpd src, Value v,
                     std::vector<int32_t>& offsets,
                     const std::vector<RegisterAssignment>& register_assign);
  void ZextDWordValue(GPRegister dest, Value v, std::vector<int32_t>& offsets,
                      const std::vector<RegisterAssignment>& register_assign);
  asmjit::x86::Mem GetDWordPtrValue(
      Value v, std::vector<int32_t>& offsets,
      const std::vector<uint64_t>& instrs,
      const std::vector<RegisterAssignment>& register_assign);
  asmjit::x86::Mem GetYMMWordPtrValue(
      Value v, std::vector<int32_t>& offsets,
      const std::vector<uint64_t>& instrs,
      const std::vector<RegisterAssignment>& register_assign);

  template <typename T>
  void MoveQWordValue(T dest, Value v, std::vector<int32_t>& offsets,
                      const std::vector<RegisterAssignment>& register_assign,
                      int32_t dynamic_stack_alloc = 0);
  template <typename T>
  void AddQWordValue(T dest, Value v, std::vector<int32_t>& offsets,
                     const std::vector<RegisterAssignment>& register_assign);
  template <typename T>
  void AndQWordValue(T dest, Value v, std::vector<int32_t>& offsets,
                     const std::vector<RegisterAssignment>& register_assign);
  template <typename T>
  void XorQWordValue(T dest, Value v, std::vector<int32_t>& offsets,
                     const std::vector<RegisterAssignment>& register_assign);
  template <typename T>
  void OrQWordValue(T dest, Value v, std::vector<int32_t>& offsets,
                    const std::vector<RegisterAssignment>& register_assign);
  template <typename T>
  void LShiftQWordValue(T dest, Value v, std::vector<int32_t>& offsets,
                        const std::vector<RegisterAssignment>& register_assign);
  template <typename T>
  void RShiftQWordValue(T dest, Value v, std::vector<int32_t>& offsets,
                        const std::vector<RegisterAssignment>& register_assign);
  template <typename T>
  void SubQWordValue(T dest, Value v, std::vector<int32_t>& offsets,
                     const std::vector<RegisterAssignment>& register_assign);
  void MulQWordValue(asmjit::x86::Gpq dest, Value v,
                     std::vector<int32_t>& offsets,
                     const std::vector<RegisterAssignment>& register_assign);
  asmjit::x86::Gpq GetQWordValue(
      Value v, std::vector<int32_t>& offsets,
      const std::vector<RegisterAssignment>& register_assign);
  void CmpQWordValue(asmjit::x86::Gpq src, Value v,
                     std::vector<int32_t>& offsets,
                     const std::vector<RegisterAssignment>& register_assign);
  template <typename T>
  void MovePtrValue(T dest, Value v, std::vector<int32_t>& offsets,
                    const std::vector<RegisterAssignment>& register_assign,
                    int32_t dynamic_stack_alloc = 0);
  asmjit::x86::Mem GetQWordPtrValue(
      Value v, std::vector<int32_t>& offsets,
      const std::vector<uint64_t>& instrs,
      const std::vector<RegisterAssignment>& register_assign);

  void MaterializeGep(asmjit::x86::Mem dest, Value v,
                      std::vector<int32_t>& offsets,
                      const std::vector<uint64_t>& instrs,
                      const std::vector<RegisterAssignment>& register_assign);
  void MaterializeGep(asmjit::x86::Gpq dest, Value v,
                      std::vector<int32_t>& offsets,
                      const std::vector<uint64_t>& instrs,
                      const std::vector<RegisterAssignment>& register_assign);
  template <typename T>
  void MoveF64Value(T dest, Value v, std::vector<int32_t>& offsets,
                    const std::vector<RegisterAssignment>& register_assign);
  void AddF64Value(asmjit::x86::Xmm dest, Value v,
                   std::vector<int32_t>& offsets,
                   const std::vector<RegisterAssignment>& register_assign);
  void SubF64Value(asmjit::x86::Xmm dest, Value v,
                   std::vector<int32_t>& offsets,
                   const std::vector<RegisterAssignment>& register_assign);
  void MulF64Value(asmjit::x86::Xmm dest, Value v,
                   std::vector<int32_t>& offsets,
                   const std::vector<RegisterAssignment>& register_assign);
  void DivF64Value(asmjit::x86::Xmm dest, Value v,
                   std::vector<int32_t>& offsets,
                   const std::vector<RegisterAssignment>& register_assign);
  asmjit::x86::Xmm GetF64Value(
      Value v, std::vector<int32_t>& offsets,
      const std::vector<RegisterAssignment>& register_assign);
  void CmpF64Value(asmjit::x86::Xmm src, Value v, std::vector<int32_t>& offsets,
                   const std::vector<RegisterAssignment>& register_assign);
  template <typename T>
  void F64ConvI64Value(T dest, Value v, std::vector<int32_t>& offsets,
                       const std::vector<RegisterAssignment>& register_assign);
  void CondBrFlag(Value v, int true_bb, int false_bb,
                  const std::vector<uint64_t>& instructions,
                  const std::vector<RegisterAssignment>& register_assign,
                  const std::vector<asmjit::Label>& basic_blocks, int next_bb);
  void CondBrF64Flag(Value v, int true_bb, int false_bb,
                     const std::vector<uint64_t>& instructions,
                     const std::vector<RegisterAssignment>& register_assign,
                     const std::vector<asmjit::Label>& basic_blocks,
                     int next_bb);
  template <typename Dest>
  void StoreCmpFlags(Opcode opcode, Dest d);
  template <typename Dest>
  void StoreF64CmpFlags(Opcode opcode, Dest d);

  asmjit::x86::Ymm GetYMMWordValue(
      Value v, std::vector<int32_t>& offsets,
      const std::vector<RegisterAssignment>& register_assign);

  asmjit::Label EmbedI32(int32_t c);
  asmjit::Label EmbedI32Vec8(int32_t c);
  asmjit::Label EmbedI32Vec8(std::array<int32_t, 8> c);

  const Program& program_;

  RegAllocImpl reg_alloc_impl_;
  asmjit::JitRuntime rt_;
  asmjit::FileLogger logger_;
  asmjit::CodeHolder code_;
  asmjit::Section* text_section_;
  asmjit::Section* data_section_;

  ExceptionErrorHandler err_handler_;
  std::unique_ptr<asmjit::x86::Assembler> asm_;

  std::vector<asmjit::Label> char_array_constants_;
  std::vector<asmjit::Label> globals_;

  std::vector<void*> external_func_addr_;
  std::vector<asmjit::Label> internal_func_labels_;
  asmjit::Label ones_;
  asmjit::Label masks_;
  asmjit::Label permute_;
  int num_floating_point_args_;
  int num_regular_args_;
  int num_stack_args_;
  void* buffer_start_;

  std::vector<std::pair<Value, Type>> regular_call_args_;
  std::vector<Value> floating_point_call_args_;

  absl::flat_hash_map<std::string, std::pair<asmjit::Label, asmjit::Label>>
      function_start_end_;

  static double compilation_time_;
};

}  // namespace kush::khir