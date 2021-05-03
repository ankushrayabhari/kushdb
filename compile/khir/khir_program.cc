#include "compile/khir/khir_program.h"

#include <cstdint>
#include <cstring>
#include <vector>

namespace kush::khir {

using Value = KhirProgram::Value;

Opcode CompTypeToOpcode(CompType c) {
  return static_cast<Opcode>(static_cast<int8_t>(c));
}

void KhirProgram::AppendValue(Value v) {
  // convert value to int32_t
  auto underlying = static_cast<int32_t>(v);

  for (int byte = 0; byte < 4; byte++) {
    instructions_.push_back((underlying >> (8 * byte)) & 0xFF);
  }
}

void KhirProgram::AppendOpcode(Opcode opcode) {
  instructions_.push_back(static_cast<int8_t>(opcode));
}

void KhirProgram::AppendCompType(CompType c) {
  instructions_.push_back(static_cast<int8_t>(c));
}

void KhirProgram::AppendLiteral(bool v) { instructions_.push_back(v ? 0 : 1); }

void KhirProgram::AppendLiteral(int8_t v) { instructions_.push_back(v); }

void KhirProgram::AppendLiteral(int16_t v) {
  for (int byte = 0; byte < 2; byte++) {
    instructions_.push_back((v >> (8 * byte)) & 0xFF);
  }
}

void KhirProgram::AppendLiteral(int32_t v) {
  for (int byte = 0; byte < 4; byte++) {
    instructions_.push_back((v >> (8 * byte)) & 0xFF);
  }
}

void KhirProgram::AppendLiteral(int64_t v) {
  for (int byte = 0; byte < 8; byte++) {
    instructions_.push_back((v >> (8 * byte)) & 0xFF);
  }
}

void KhirProgram::AppendLiteral(double v) {
  // Non-portable way to convert double to an IEEE-754 64bit representation
  uint64_t bytes;
  static_assert(sizeof(uint64_t) == sizeof(double));
  std::memcpy(&bytes, &v, sizeof(bytes));

  for (int byte = 0; byte < 8; byte++) {
    instructions_.push_back((bytes >> (8 * byte)) & 0xFF);
  }
}

Value KhirProgram::LNotI1(Value v) {
  auto offset = instructions_.size();
  AppendOpcode(Opcode::LNOT_I1);
  AppendValue(v);
  return Value(offset);
}

Value KhirProgram::CmpI1(CompType cmp, Value v1, Value v2) {
  auto offset = instructions_.size();
  AppendOpcode(Opcode::CMP_I1);
  AppendCompType(cmp);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::ConstI1(bool v) {
  auto offset = instructions_.size();
  AppendOpcode(Opcode::CONST_I1);
  AppendLiteral(v);
  return Value(offset);
}

Value KhirProgram::AddI8(Value v1, Value v2) {
  auto offset = instructions_.size();
  AppendOpcode(Opcode::ADD_I8);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::MulI8(Value v1, Value v2) {
  auto offset = instructions_.size();
  AppendOpcode(Opcode::MUL_I8);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::DivI8(Value v1, Value v2) {
  auto offset = instructions_.size();
  AppendOpcode(Opcode::DIV_I8);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::SubI8(Value v1, Value v2) {
  auto offset = instructions_.size();
  AppendOpcode(Opcode::SUB_I8);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::CmpI8(CompType cmp, Value v1, Value v2) {
  auto offset = instructions_.size();
  AppendOpcode(Opcode::CMP_I8);
  AppendCompType(cmp);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::ConstI8(int8_t v) {
  auto offset = instructions_.size();
  AppendOpcode(Opcode::CONST_I8);
  AppendLiteral(v);
  return Value(offset);
}

}  // namespace kush::khir