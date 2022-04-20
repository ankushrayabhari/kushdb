#include "khir/program.h"

#include <utility>
#include <vector>

#include "khir/type_manager.h"
#include "khir/value.h"

namespace kush::khir {

StructConstant::StructConstant(khir::Type type, absl::Span<const Value> fields)
    : type_(type), fields_(fields.begin(), fields.end()) {}

khir::Type StructConstant::Type() const { return type_; }

absl::Span<const Value> StructConstant::Fields() const { return fields_; }

ArrayConstant::ArrayConstant(khir::Type type, absl::Span<const Value> elements)
    : type_(type), elements_(elements.begin(), elements.end()) {}

khir::Type ArrayConstant::Type() const { return type_; }

absl::Span<const Value> ArrayConstant::Elements() const { return elements_; }

Global::Global(khir::Type type, khir::Type ptr_to_type, Value init)
    : type_(type), ptr_to_type_(ptr_to_type), init_(init) {}

Type Global::Type() const { return type_; }

Value Global::InitialValue() const { return init_; }

Type Global::PointerToType() const { return ptr_to_type_; }

BasicBlock::BasicBlock(std::vector<std::pair<int, int>> segments,
                       const std::vector<int>& succ,
                       const std::vector<int>& pred)
    : segments_(std::move(segments)),
      succ_(std::move(succ)),
      pred_(std::move(pred)) {}

const std::vector<std::pair<int, int>>& BasicBlock::Segments() const {
  return segments_;
}

const std::vector<int>& BasicBlock::Successors() const { return succ_; }

const std::vector<int>& BasicBlock::Predecessors() const { return pred_; }

void BasicBlock::SetSuccessors(std::vector<int> succ) {
  succ_ = std::move(succ);
}

void BasicBlock::SetPredecessors(std::vector<int> pred) {
  pred_ = std::move(pred);
}

Function::Function(std::string name, khir::Type type, bool pub,
                   std::vector<uint64_t> instructions,
                   std::vector<BasicBlock> basic_blocks)
    : external_(false),
      addr_(nullptr),
      name_(name),
      type_(type),
      public_(pub),
      instructions_(std::move(instructions)),
      basic_blocks_(std::move(basic_blocks)) {}

Function::Function(std::string name, khir::Type type, void* addr)
    : external_(true), addr_(addr), name_(name), type_(type), public_(false) {}

std::string_view Function::Name() const { return name_; }

khir::Type Function::Type() const { return type_; }

// External
bool Function::External() const { return external_; }

void* Function::Addr() const { return addr_; }

// Internal
bool Function::Public() const { return public_; }

const std::vector<uint64_t>& Function::Instrs() const { return instructions_; }

const std::vector<BasicBlock>& Function::BasicBlocks() const {
  return basic_blocks_;
}

Program::Program(
    khir::TypeManager type_manager, std::vector<Function> functions,
    std::vector<uint64_t> constant_instrs, std::vector<void*> ptr_constants,
    std::vector<uint64_t> i64_constants, std::vector<double> f64_constants,
    std::vector<std::string> char_array_constants,
    std::vector<StructConstant> struct_constants,
    std::vector<ArrayConstant> array_constants, std::vector<Global> globals)
    : type_manager_(std::move(type_manager)),
      functions_(std::move(functions)),
      constant_instrs_(std::move(constant_instrs)),
      ptr_constants_(std::move(ptr_constants)),
      i64_constants_(std::move(i64_constants)),
      f64_constants_(std::move(f64_constants)),
      char_array_constants_(std::move(char_array_constants)),
      struct_constants_(std::move(struct_constants)),
      array_constants_(std::move(array_constants)),
      globals_(std::move(globals)) {}

const khir::TypeManager& Program::TypeManager() const { return type_manager_; }

const std::vector<Function>& Program::Functions() const { return functions_; }

const std::vector<uint64_t>& Program::ConstantInstrs() const {
  return constant_instrs_;
}

const std::vector<void*>& Program::PtrConstants() const {
  return ptr_constants_;
}

const std::vector<uint64_t>& Program::I64Constants() const {
  return i64_constants_;
}

const std::vector<double>& Program::F64Constants() const {
  return f64_constants_;
}

const std::vector<std::string>& Program::CharArrayConstants() const {
  return char_array_constants_;
}

const std::vector<StructConstant>& Program::StructConstants() const {
  return struct_constants_;
}

const std::vector<ArrayConstant>& Program::ArrayConstants() const {
  return array_constants_;
}

const std::vector<Global>& Program::Globals() const { return globals_; }

}  // namespace kush::khir
