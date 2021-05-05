#include "compile/khir/khir_program.h"

#include <cstdint>
#include <cstring>
#include <vector>

namespace kush::khir {

Type GetPointedToType(const Type& t) {
  const auto& v = static_cast<const absl::InlinedVector<int8_t, 4>&>(t);
  return Type(absl::InlinedVector<int8_t, 4>(v.begin() + 5, v.end()));
}

Type GetArrayElementType(const Type& t) {
  const auto& v = static_cast<const absl::InlinedVector<int8_t, 4>&>(t);
  return Type(absl::InlinedVector<int8_t, 4>(v.begin() + 9, v.end()));
}

Type GetStructFieldType(const Type& t, int32_t idx) {
  // 1 byte for TypeID
  // 4 bytes for type length
  // 4 bytes for # of fields
  // v.size() bytes for each element type
  const auto& v = static_cast<const absl::InlinedVector<int8_t, 4>&>(t);

  int32_t num_fields = 0;
  for (int byte = 4; byte < 8; byte++) {
    int32_t upshifted = v[byte];
    num_fields |= upshifted << (8 * (3 - byte));
  }

  int32_t last_field = 9;
  for (int i = 0; i < idx; i++) {
    auto id_int = v[last_field];
    TypeID id = static_cast<TypeID>(id_int);

    switch (id) {
      case TypeID::I1:
      case TypeID::I8:
      case TypeID::I16:
      case TypeID::I32:
      case TypeID::I64:
      case TypeID::F64:
      case TypeID::VOID: {
        last_field += 1;
        break;
      }

      case TypeID::POINTER:
      case TypeID::ARRAY:
      case TypeID::STRUCT:
      case TypeID::FUNCTION: {
        int32_t len = 0;
        for (int byte = last_field + 1; byte < last_field + 5; byte++) {
          int32_t upshifted = v[byte];
          len |= upshifted << (8 * (3 - byte));
        }

        last_field += len;
        break;
      }
    }
  }

  auto id_int = v[last_field];
  TypeID id = static_cast<TypeID>(id_int);

  switch (id) {
    case TypeID::I1:
    case TypeID::I8:
    case TypeID::I16:
    case TypeID::I32:
    case TypeID::I64:
    case TypeID::F64:
    case TypeID::VOID: {
      return Type(absl::InlinedVector<int8_t, 4>(v.begin() + last_field,
                                                 v.begin() + last_field + 1));
    }

    case TypeID::POINTER:
    case TypeID::ARRAY:
    case TypeID::STRUCT:
    case TypeID::FUNCTION: {
      int32_t len = 0;
      for (int byte = last_field + 1; byte < last_field + 5; byte++) {
        int32_t upshifted = v[byte];
        len |= upshifted << (8 * (3 - byte));
      }

      return Type(absl::InlinedVector<int8_t, 4>(v.begin() + last_field,
                                                 v.begin() + last_field + len));
    }
  }
}

TypeID GetTypeID(const Type& t) {
  const auto& v = static_cast<const absl::InlinedVector<int8_t, 4>&>(t);
  return static_cast<TypeID>(v[0]);
}

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

void KhirProgram::AppendType(const Type& t) {
  const auto& v = static_cast<const absl::InlinedVector<int8_t, 4>&>(t);
  instructions_per_function_[current_function_].insert(
      instructions_per_function_[current_function_].end(), v.begin(), v.end());
}

absl::Span<int8_t> KhirProgram::GetType(int32_t offset) {
  auto id_int = GetI8Literal(offset);
  TypeID id = static_cast<TypeID>(id_int);

  switch (id) {
    case TypeID::I1:
    case TypeID::I8:
    case TypeID::I16:
    case TypeID::I32:
    case TypeID::I64:
    case TypeID::F64:
    case TypeID::VOID: {
      auto it = instructions_per_function_[current_function_].data() + offset;
      return absl::Span<int8_t>(it, 1);
    }

    case TypeID::POINTER:
    case TypeID::ARRAY:
    case TypeID::STRUCT:
    case TypeID::FUNCTION: {
      auto len = GetI32Literal(offset + 1);
      auto it = instructions_per_function_[current_function_].data() + offset;
      return absl::Span<int8_t>(it, len);
    }
  }
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

// Types
Type KhirProgram::VoidType() {
  auto id = static_cast<int8_t>(TypeID::VOID);
  return Type(absl::InlinedVector<int8_t, 4>(id));
}

Type KhirProgram::I1Type() {
  auto id = static_cast<int8_t>(TypeID::I1);
  return Type(absl::InlinedVector<int8_t, 4>(id));
}

Type KhirProgram::I8Type() {
  auto id = static_cast<int8_t>(TypeID::I8);
  return Type(absl::InlinedVector<int8_t, 4>(id));
}

Type KhirProgram::I16Type() {
  auto id = static_cast<int8_t>(TypeID::I16);
  return Type(absl::InlinedVector<int8_t, 4>(id));
}

Type KhirProgram::I32Type() {
  auto id = static_cast<int8_t>(TypeID::I32);
  return Type(absl::InlinedVector<int8_t, 4>(id));
}

Type KhirProgram::I64Type() {
  auto id = static_cast<int8_t>(TypeID::I64);
  return Type(absl::InlinedVector<int8_t, 4>(id));
}

Type KhirProgram::F64Type() {
  auto id = static_cast<int8_t>(TypeID::F64);
  return Type(absl::InlinedVector<int8_t, 4>(id));
}

Type KhirProgram::StructType(absl::Span<const Type> types,
                             std::string_view name) {
  // 1 byte for TypeID
  // 4 bytes for type length
  // 4 bytes for # of fields
  // v.size() bytes for each element type
  int32_t type_len = 1 + 4 + 4;
  for (const Type& t : types) {
    const auto& v = static_cast<const absl::InlinedVector<int8_t, 4>&>(t);
    type_len += v.size();
  }

  absl::InlinedVector<int8_t, 4> result;
  result.reserve(type_len);

  result.push_back(static_cast<int8_t>(TypeID::STRUCT));
  result.push_back(type_len);

  int32_t num_fields = types.size();
  for (int byte = 0; byte < 4; byte++) {
    result.push_back((num_fields >> (8 * byte)) & 0xFF);
  }

  for (const Type& t : types) {
    const auto& v = static_cast<const absl::InlinedVector<int8_t, 4>&>(t);
    result.insert(result.end(), v.begin(), v.end());
  }

  Type output = Type(result);

  if (name.empty()) {
    return output;
  }

  if (named_structs_.contains(name)) {
    throw std::runtime_error("Already defined struct.");
  }
  named_structs_[name] = output;
  return output;
}

Type KhirProgram::GetStructType(std::string_view name) {
  auto it = named_structs_.find(name);
  if (it == named_structs_.end()) {
    throw std::runtime_error("Unknown struct.");
  }

  return it->second;
}

Type KhirProgram::PointerType(Type t) {
  const auto& v = static_cast<const absl::InlinedVector<int8_t, 4>&>(t);

  // 1 byte for TypeID
  // 4 bytes for type length
  // v.size() bytes for element type
  int32_t type_len = 1 + 4 + v.size();

  absl::InlinedVector<int8_t, 4> result;
  result.reserve(type_len);

  result.push_back(static_cast<int8_t>(TypeID::POINTER));
  for (int byte = 0; byte < 4; byte++) {
    result.push_back((type_len >> (8 * byte)) & 0xFF);
  }

  result.insert(result.end(), v.begin(), v.end());
  return Type(result);
}

Type KhirProgram::ArrayType(Type t, int len) {
  const auto& v = static_cast<const absl::InlinedVector<int8_t, 4>&>(t);

  // 1 byte for TypeID
  // 4 bytes for type length
  // 4 bytes for array length
  // v.size() bytes for element type
  int32_t type_len = 1 + 4 + 4 + v.size();

  absl::InlinedVector<int8_t, 4> result;
  result.reserve(type_len);

  result.push_back(static_cast<int8_t>(TypeID::ARRAY));
  for (int byte = 0; byte < 4; byte++) {
    result.push_back((type_len >> (8 * byte)) & 0xFF);
  }

  int32_t array_len = len;
  for (int byte = 0; byte < 4; byte++) {
    result.push_back((array_len >> (8 * byte)) & 0xFF);
  }
  result.insert(result.end(), v.begin(), v.end());
  return Type(result);
}

Type KhirProgram::FunctionType(Type return_type, absl::Span<const Type> args) {
  const auto& return_type_arr =
      static_cast<const absl::InlinedVector<int8_t, 4>&>(return_type);

  // 1 byte for TypeID
  // 4 bytes for type length
  // # of bytes byte for return type
  // 4 bytes for # of args
  // bytes of all args
  int32_t type_len = 1 + 4 + return_type_arr.size() + 4;
  for (const Type& t : args) {
    const auto& v = static_cast<const absl::InlinedVector<int8_t, 4>&>(t);
    type_len += v.size();
  }

  absl::InlinedVector<int8_t, 4> result;
  result.reserve(type_len);
  result.push_back(static_cast<int8_t>(TypeID::FUNCTION));
  for (int byte = 0; byte < 4; byte++) {
    result.push_back((type_len >> (8 * byte)) & 0xFF);
  }

  result.insert(result.end(), return_type_arr.begin(), return_type_arr.end());

  int32_t num_fields = args.size();
  for (int byte = 0; byte < 4; byte++) {
    result.push_back((num_fields >> (8 * byte)) & 0xFF);
  }
  for (const Type& t : args) {
    const auto& v = static_cast<const absl::InlinedVector<int8_t, 4>&>(t);
    result.insert(result.end(), v.begin(), v.end());
  }
  return Type(result);
}

Value KhirProgram::SizeOf(Type t) {
  const auto& v = static_cast<const absl::InlinedVector<int8_t, 4>&>(t);
  TypeID id = static_cast<TypeID>(v[0]);
  if (id != TypeID::STRUCT) {
    throw std::runtime_error("SizeOf not allowed on non-struct types.");
  }

  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::SIZEOF);
  AppendType(I64Type());
  AppendType(t);
  return Value(offset);
}

Value KhirProgram::Alloca(Type t) {
  auto offset = instructions_per_function_[current_function_].size();

  AppendOpcode(Opcode::ALLOCA);
  AppendType(PointerType(t));
  AppendType(t);

  return Value(offset);
}

Value KhirProgram::NullPtr(Type t) {
  auto offset = instructions_per_function_[current_function_].size();

  AppendOpcode(Opcode::CONST_I64);
  AppendType(PointerType(t));
  AppendLiteral(static_cast<int64_t>(0));

  return Value(offset);
}

Value KhirProgram::GetElementPtr(Type content_type, Value ptr,
                                 absl::Span<const int32_t> idx) {
  // Compute result type
  Type last_type = PointerType(content_type);
  for (int x : idx) {
    switch (GetTypeID(last_type)) {
      case TypeID::I1:
      case TypeID::I8:
      case TypeID::I16:
      case TypeID::I32:
      case TypeID::I64:
      case TypeID::F64:
      case TypeID::VOID:
      case TypeID::FUNCTION: {
        throw std::runtime_error("Cannot index into value type.");
      }

      case TypeID::POINTER: {
        last_type = GetPointedToType(last_type);
        break;
      }

      case TypeID::ARRAY: {
        last_type = GetArrayElementType(last_type);
        break;
      }

      case TypeID::STRUCT: {
        last_type = GetStructFieldType(last_type, x);
        break;
      }
    }
  }

  auto offset = instructions_per_function_[current_function_].size();

  AppendOpcode(Opcode::GEP);
  AppendType(PointerType(last_type));
  AppendValue(ptr);
  for (int32_t i : idx) {
    AppendLiteral(i);
  }

  return Value(offset);
}

Value KhirProgram::PointerCast(Value v, Type t) {
  auto offset = instructions_per_function_[current_function_].size();

  AppendOpcode(Opcode::PTR_CAST);
  AppendType(t);
  AppendValue(v);

  return Value(offset);
}

void KhirProgram::Store(Value ptr, Value v) {
  AppendOpcode(Opcode::STORE);
  AppendType(VoidType());
  AppendValue(ptr);
  AppendValue(v);
}

Value KhirProgram::Load(Value ptr) {
  auto offset = instructions_per_function_[current_function_].size();
  auto type = TypeOf(ptr);
  AppendOpcode(Opcode::LOAD);
  AppendType(GetPointedToType(type));
  AppendValue(ptr);
  return Value(offset);
}

Type KhirProgram::TypeOf(Value value) {
  return Type(absl::InlinedVector<int8_t, 4>(static_cast<int32_t>(value) +
                                             sizeof(Opcode)));
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
  AppendType(VoidType());
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
  AppendType(VoidType());
  AppendValue(cond);
  AppendBasicBlockIdx(block_idx1);
  AppendBasicBlockIdx(block_idx2);

  basic_blocks_per_function_[current_function_][current_block_].Terminate(
      offset);
}

Value KhirProgram::Phi(Type type) {
  int32_t phi_id = phi_values_.size();
  phi_values_.emplace_back();

  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::PHI);
  AppendType(type);
  AppendLiteral(phi_id);
  return Value(offset);
}

void KhirProgram::AddToPhi(Value phi, Value v, BasicBlock b) {
  auto offset = static_cast<int32_t>(phi);
  auto type = GetType(offset + 1);
  int32_t phi_id_offset = offset + 1 + type.size();
  int32_t phi_id = GetI32Literal(phi_id_offset);
  auto [func_idx, block_idx] = static_cast<std::pair<int32_t, int32_t>>(b);
  phi_values_[phi_id].emplace_back(block_idx, v);
}

Value KhirProgram::LNotI1(Value v) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::LNOT_I1);
  AppendType(I1Type());
  AppendValue(v);
  return Value(offset);
}

Value KhirProgram::CmpI1(CompType cmp, Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::CMP_I1);
  AppendType(I1Type());
  AppendCompType(cmp);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::ConstI1(bool v) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::CONST_I1);
  AppendType(I1Type());
  AppendLiteral(v);
  return Value(offset);
}

Value KhirProgram::AddI8(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::ADD_I8);
  AppendType(I8Type());
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::MulI8(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::MUL_I8);
  AppendType(I8Type());
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::DivI8(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::DIV_I8);
  AppendType(I8Type());
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::SubI8(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::SUB_I8);
  AppendType(I8Type());
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::CmpI8(CompType cmp, Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::CMP_I8);
  AppendType(I1Type());
  AppendCompType(cmp);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::ConstI8(int8_t v) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::CONST_I8);
  AppendType(I1Type());
  AppendLiteral(v);
  return Value(offset);
}

Value KhirProgram::AddI16(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::ADD_I16);
  AppendType(I16Type());
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::MulI16(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::MUL_I16);
  AppendType(I16Type());
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::DivI16(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::DIV_I16);
  AppendType(I16Type());
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::SubI16(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::SUB_I16);
  AppendType(I16Type());
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::CmpI16(CompType cmp, Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::CMP_I16);
  AppendType(I1Type());
  AppendCompType(cmp);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::ConstI16(int16_t v) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::CONST_I16);
  AppendType(I16Type());
  AppendLiteral(v);
  return Value(offset);
}

Value KhirProgram::AddI32(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::ADD_I32);
  AppendType(I32Type());
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::MulI32(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::MUL_I32);
  AppendType(I32Type());
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::DivI32(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::DIV_I32);
  AppendType(I32Type());
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::SubI32(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::SUB_I32);
  AppendType(I32Type());
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::CmpI32(CompType cmp, Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::CMP_I32);
  AppendType(I1Type());
  AppendCompType(cmp);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::ConstI32(int32_t v) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::CONST_I32);
  AppendType(I32Type());
  AppendLiteral(v);
  return Value(offset);
}

// I64
Value KhirProgram::AddI64(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::ADD_I64);
  AppendType(I64Type());
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::MulI64(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::MUL_I64);
  AppendType(I64Type());
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::DivI64(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::DIV_I64);
  AppendType(I64Type());
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::SubI64(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::SUB_I64);
  AppendType(I64Type());
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::CmpI64(CompType cmp, Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::CMP_I64);
  AppendType(I1Type());
  AppendCompType(cmp);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::ZextI64(Value v) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::ZEXT_I64);
  AppendType(I64Type());
  AppendValue(v);
  return Value(offset);
}

Value KhirProgram::F64ConversionI64(Value v) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::F64_CONV_I64);
  AppendType(I64Type());
  AppendValue(v);
  return Value(offset);
}

Value KhirProgram::ConstI64(int64_t v) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::CONST_I64);
  AppendType(I64Type());
  AppendLiteral(v);
  return Value(offset);
}

Value KhirProgram::AddF64(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::ADD_F64);
  AppendType(F64Type());
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::MulF64(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::MUL_F64);
  AppendType(F64Type());
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::DivF64(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::DIV_F64);
  AppendType(F64Type());
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::SubF64(Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::SUB_F64);
  AppendType(F64Type());
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::CmpF64(CompType cmp, Value v1, Value v2) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::CMP_F64);
  AppendType(I1Type());
  AppendCompType(cmp);
  AppendValue(v1);
  AppendValue(v2);
  return Value(offset);
}

Value KhirProgram::ConstF64(double v) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::CONST_F64);
  AppendType(F64Type());
  AppendLiteral(v);
  return Value(offset);
}

Value KhirProgram::CastSignedIntToF64(Value v) {
  auto offset = instructions_per_function_[current_function_].size();
  AppendOpcode(Opcode::I_CONV_F64);
  AppendType(F64Type());
  AppendValue(v);
  return Value(offset);
}

}  // namespace kush::khir