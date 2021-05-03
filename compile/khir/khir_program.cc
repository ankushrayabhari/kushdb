#include "compile/khir/khir_program.h"

#include <cstdint>
#include <cstring>
#include <vector>

namespace kush::khir {

BasicBlockImpl::BasicBlockImpl(int32_t current_function)
    : begin_offset_(-1), end_offset_(-1), current_function_(current_function) {}

void BasicBlockImpl::Terminate(int32_t offset) { end_offset_ = offset; }

void BasicBlockImpl::Codegen(int32_t offset) { begin_offset_ = offset; }

int32_t BasicBlockImpl::CurrentFunction() const { return current_function_; }

bool BasicBlockImpl::operator==(const BasicBlockImpl& other) const {
  return begin_offset_ == other.begin_offset_ &&
         end_offset_ == other.end_offset_;
}

bool BasicBlockImpl::IsTerminated() const { return end_offset_ >= 0; }

bool BasicBlockImpl::IsEmpty() const { return begin_offset_ == -1; }

void KhirProgram::AppendValue(Value v) {
  // convert value to int32_t
  auto underlying = static_cast<int32_t>(v);

  for (int byte = 0; byte < 4; byte++) {
    instructions_per_function_[current_function_].push_back(
        (underlying >> (8 * byte)) & 0xFF);
  }
}

void KhirProgram::AppendBasicBlockIdx(int32_t b) { AppendLiteral(b); }

int32_t KhirProgram::GetBasicBlockIdx(int32_t offset) {
  return GetI32Literal(offset);
}

void KhirProgram::AppendOpcode(Opcode opcode) {
  instructions_per_function_[current_function_].push_back(
      static_cast<int8_t>(opcode));
}

Opcode KhirProgram::GetOpcode(int32_t offset) {
  int8_t literal = GetI8Literal(offset);
  return static_cast<Opcode>(literal);
}

void KhirProgram::AppendCompType(CompType c) {
  instructions_per_function_[current_function_].push_back(
      static_cast<int8_t>(c));
}

CompType KhirProgram::GetCompType(int32_t offset) {
  int8_t literal = GetI8Literal(offset);
  return static_cast<CompType>(literal);
}

void KhirProgram::AppendLiteral(bool v) {
  instructions_per_function_[current_function_].push_back(v ? 0 : 1);
}

bool KhirProgram::GetBoolLiteral(int32_t offset) {
  int8_t literal = GetI8Literal(offset);
  return literal == 0 ? false : true;
}

void KhirProgram::AppendLiteral(int8_t v) {
  instructions_per_function_[current_function_].push_back(v);
}

int8_t KhirProgram::GetI8Literal(int32_t offset) {
  return instructions_per_function_[current_function_][offset];
}

void KhirProgram::AppendLiteral(int16_t v) {
  for (int byte = 0; byte < 2; byte++) {
    instructions_per_function_[current_function_].push_back((v >> (8 * byte)) &
                                                            0xFF);
  }
}

int16_t KhirProgram::GetI16Literal(int32_t offset) {
  int16_t result = 0;
  for (int byte = 0; byte < 2; byte++) {
    int16_t upshifted =
        instructions_per_function_[current_function_][offset + byte];
    result |= upshifted << (8 * (1 - byte));
  }
  return result;
}

void KhirProgram::AppendLiteral(int32_t v) {
  for (int byte = 0; byte < 4; byte++) {
    instructions_per_function_[current_function_].push_back((v >> (8 * byte)) &
                                                            0xFF);
  }
}

int32_t KhirProgram::GetI32Literal(int32_t offset) {
  int32_t result = 0;
  for (int byte = 0; byte < 4; byte++) {
    int32_t upshifted =
        instructions_per_function_[current_function_][offset + byte];
    result |= upshifted << (8 * (3 - byte));
  }
  return result;
}

void KhirProgram::AppendLiteral(int64_t v) {
  for (int byte = 0; byte < 8; byte++) {
    instructions_per_function_[current_function_].push_back((v >> (8 * byte)) &
                                                            0xFF);
  }
}

int64_t KhirProgram::GetI64Literal(int32_t offset) {
  int64_t result = 0;
  for (int byte = 0; byte < 8; byte++) {
    int64_t upshifted =
        instructions_per_function_[current_function_][offset + byte];
    result |= upshifted << (8 * (3 - byte));
  }
  return result;
}

void KhirProgram::AppendLiteral(double v) {
  // Non-portable way to convert double to an IEEE-754 64bit representation
  int64_t bytes;
  static_assert(sizeof(int64_t) == sizeof(double));
  std::memcpy(&bytes, &v, sizeof(bytes));
  AppendLiteral(bytes);
}

double KhirProgram::GetF64Literal(int32_t offset) {
  int64_t bytes = GetI64Literal(offset);
  double result;
  static_assert(sizeof(int64_t) == sizeof(double));
  std::memcpy(&result, &bytes, sizeof(result));
  return result;
}

BasicBlock KhirProgram::GenerateBlock() {
  auto offset = basic_blocks_per_function_[current_function_].size();
  basic_blocks_per_function_[current_function_].emplace_back(current_function_);
  return BasicBlock({current_function_, offset});
}

BasicBlock KhirProgram::CurrentBlock() {
  return BasicBlock({current_function_, current_block_});
}

bool KhirProgram::IsTerminated(BasicBlock b) {
  auto [func_idx, block_idx] = static_cast<std::pair<int32_t, int32_t>>(b);
  return basic_blocks_per_function_[func_idx][block_idx].IsTerminated();
}

void KhirProgram::SetCurrentBlock(BasicBlock b) {
  auto [func_idx, block_idx] = static_cast<std::pair<int32_t, int32_t>>(b);
  current_function_ = func_idx;
  current_block_ = block_idx;

  auto& basic_block = basic_blocks_per_function_[func_idx][block_idx];
  if (basic_block.IsTerminated()) {
    throw std::runtime_error("Cannot switch to a terminated basic block");
  }

  if (basic_block.IsEmpty()) {
    basic_block.Codegen(instructions_per_function_[func_idx].size());
  }
}

void KhirProgram::Branch(BasicBlock b) {
  auto offset = instructions_per_function_[current_function_].size();
  auto [func_idx, block_idx] = static_cast<std::pair<int32_t, int32_t>>(b);
  if (func_idx != current_function_) {
    throw std::runtime_error("Cannot branch outside of function.");
  }

  AppendOpcode(Opcode::BR);
  AppendBasicBlockIdx(block_idx);

  basic_blocks_per_function_[current_function_][current_block_].Terminate(
      offset);
}

void KhirProgram::Branch(Value cond, BasicBlock b1, BasicBlock b2) {
  auto offset = instructions_per_function_[current_function_].size();
  auto [func_idx1, block_idx1] = static_cast<std::pair<int32_t, int32_t>>(b1);
  auto [func_idx2, block_idx2] = static_cast<std::pair<int32_t, int32_t>>(b2);
  if (func_idx1 != current_function_ || func_idx2 != current_function_) {
    throw std::runtime_error("Cannot branch outside of function.");
  }

  AppendOpcode(Opcode::COND_BR);
  AppendValue(cond);
  AppendBasicBlockIdx(block_idx1);
  AppendBasicBlockIdx(block_idx2);

  basic_blocks_per_function_[current_function_][current_block_].Terminate(
      offset);
}

Value KhirProgram::Phi(/* Type type */) {
  int32_t phi_id = phi_values_.size();
  phi_values_.emplace_back();

  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::PHI);
  AppendLiteral(phi_id);
  return Value(offset);
}

void KhirProgram::AddToPhi(Value phi, Value v, BasicBlock b) {
  int32_t phi_id_offset = static_cast<int32_t>(phi) + sizeof(Value);
  int32_t phi_id = GetI32Literal(phi_id_offset);
  auto [func_idx, block_idx] = static_cast<std::pair<int32_t, int32_t>>(b);
  phi_values_[phi_id].emplace_back(block_idx, v);
}

Value KhirProgram::LNotI1(Value v) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::LNOT_I1);
  AppendValue(v);
  return Value(offset);
}

Value KhirProgram::CmpI1(CompType cmp, Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::CMP_I1);
  AppendCompType(cmp);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::ConstI1(bool v) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::CONST_I1);
  AppendLiteral(v);
  return Value(offset);
}

Value KhirProgram::AddI8(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::ADD_I8);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::MulI8(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::MUL_I8);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::DivI8(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::DIV_I8);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::SubI8(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::SUB_I8);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::CmpI8(CompType cmp, Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::CMP_I8);
  AppendCompType(cmp);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::ConstI8(int8_t v) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::CONST_I8);
  AppendLiteral(v);
  return Value(offset);
}

Value KhirProgram::AddI16(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::ADD_I16);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::MulI16(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::MUL_I16);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::DivI16(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::DIV_I16);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::SubI16(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::SUB_I16);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::CmpI16(CompType cmp, Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::CMP_I16);
  AppendCompType(cmp);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::ConstI16(int16_t v) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::CONST_I16);
  AppendLiteral(v);
  return Value(offset);
}

Value KhirProgram::AddI32(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::ADD_I32);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::MulI32(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::MUL_I32);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::DivI32(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::DIV_I32);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::SubI32(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::SUB_I32);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::CmpI32(CompType cmp, Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::CMP_I32);
  AppendCompType(cmp);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::ConstI32(int32_t v) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::CONST_I32);
  AppendLiteral(v);
  return Value(offset);
}

// I64
Value KhirProgram::AddI64(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::ADD_I64);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::MulI64(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::MUL_I64);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::DivI64(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::DIV_I64);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::SubI64(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::SUB_I64);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::CmpI64(CompType cmp, Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::CMP_I64);
  AppendCompType(cmp);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::ZextI64(Value v) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::ZEXT_I64);
  AppendValue(v);
  return Value(offset);
}

Value KhirProgram::F64ConversionI64(Value v) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::F64_CONV_I64);
  AppendValue(v);
  return Value(offset);
}

Value KhirProgram::ConstI64(int64_t v) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::CONST_I64);
  AppendLiteral(v);
  return Value(offset);
}

Value KhirProgram::AddF64(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::ADD_F64);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::MulF64(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::MUL_F64);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::DivF64(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::DIV_F64);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::SubF64(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::SUB_F64);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::CmpF64(CompType cmp, Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::CMP_F64);
  AppendCompType(cmp);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::ConstF64(double v) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::CONST_F64);
  AppendLiteral(v);
  return Value(offset);
}

Value KhirProgram::CastSignedIntToF64(Value v) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::I_CONV_F64);
  AppendValue(v);
  return Value(offset);
}

}  // namespace kush::khir