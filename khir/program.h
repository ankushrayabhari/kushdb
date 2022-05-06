#pragma once

#include <utility>
#include <vector>

#include "khir/type_manager.h"
#include "khir/value.h"

namespace kush::khir {

class StructConstant {
 public:
  StructConstant(khir::Type type, absl::Span<const Value> fields);
  khir::Type Type() const;
  absl::Span<const Value> Fields() const;

 private:
  khir::Type type_;
  std::vector<Value> fields_;
};

class ArrayConstant {
 public:
  ArrayConstant(khir::Type type, absl::Span<const Value> elements);
  khir::Type Type() const;
  absl::Span<const Value> Elements() const;

 private:
  khir::Type type_;
  std::vector<Value> elements_;
};

class Global {
 public:
  Global(khir::Type type, khir::Type ptr_to_type, Value init);
  khir::Type PointerToType() const;
  khir::Type Type() const;
  Value InitialValue() const;

 private:
  khir::Type type_;
  khir::Type ptr_to_type_;
  Value init_;
};

class BasicBlock {
 public:
  BasicBlock(std::vector<std::pair<int, int>> segments,
             const std::vector<int>& succ, const std::vector<int>& pred);

  const std::vector<std::pair<int, int>>& Segments() const;
  const std::vector<int>& Successors() const;
  const std::vector<int>& Predecessors() const;
  std::vector<std::pair<int, int>>& Segments();
  void SetSuccessors(std::vector<int> succ);
  void SetPredecessors(std::vector<int> pred);

 private:
  std::vector<std::pair<int, int>> segments_;
  std::vector<int> succ_;
  std::vector<int> pred_;
};

class Function {
 public:
  Function(std::string name, Type type, bool pub,
           std::vector<uint64_t> instructions,
           std::vector<BasicBlock> basic_blocks);
  Function(std::string name, Type type, void* addr);

  std::string_view Name() const;
  khir::Type Type() const;

  // External
  bool External() const;
  void* Addr() const;

  // Internal
  bool Public() const;
  const std::vector<uint64_t>& Instrs() const;
  const std::vector<BasicBlock>& BasicBlocks() const;

 private:
  bool external_;
  void* addr_;

  std::string name_;
  khir::Type type_;
  bool public_;
  std::vector<uint64_t> instructions_;
  std::vector<BasicBlock> basic_blocks_;
};

class Program {
 public:
  Program(khir::TypeManager type_manager, std::vector<Function> functions,
          std::vector<uint64_t> constant_instrs,
          std::vector<void*> ptr_constants, std::vector<uint64_t> i64_constants,
          std::vector<double> f64_constants,
          std::vector<std::string> char_array_constants,
          std::vector<StructConstant> struct_constants,
          std::vector<ArrayConstant> array_constants,
          std::vector<Global> globals,
          std::vector<std::array<int32_t, 4>> vec4_constants,
          std::vector<std::array<int32_t, 8>> vec8_constants);

  const khir::TypeManager& TypeManager() const;
  const std::vector<Function>& Functions() const;
  const std::vector<uint64_t>& ConstantInstrs() const;
  const std::vector<void*>& PtrConstants() const;
  const std::vector<uint64_t>& I64Constants() const;
  const std::vector<std::array<int32_t, 4>>& I32Vec4Constants() const;
  const std::vector<std::array<int32_t, 8>>& I32Vec8Constants() const;
  const std::vector<double>& F64Constants() const;
  const std::vector<std::string>& CharArrayConstants() const;
  const std::vector<StructConstant>& StructConstants() const;
  const std::vector<ArrayConstant>& ArrayConstants() const;
  const std::vector<Global>& Globals() const;

 private:
  khir::TypeManager type_manager_;
  std::vector<Function> functions_;
  std::vector<uint64_t> constant_instrs_;
  std::vector<void*> ptr_constants_;
  std::vector<uint64_t> i64_constants_;
  std::vector<double> f64_constants_;
  std::vector<std::string> char_array_constants_;
  std::vector<StructConstant> struct_constants_;
  std::vector<ArrayConstant> array_constants_;
  std::vector<Global> globals_;
  std::vector<std::array<int32_t, 4>> vec4_constants_;
  std::vector<std::array<int32_t, 8>> vec8_constants_;
};

class ProgramTranslator {
 public:
  virtual void Translate(const Program& program) = 0;
};

}  // namespace kush::khir
