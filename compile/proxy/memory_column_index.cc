#include "compile/proxy/memory_column_index.h"

#include <functional>
#include <utility>
#include <vector>

#include "compile/proxy/control_flow/if.h"
#include "compile/proxy/control_flow/loop.h"
#include "compile/proxy/evaluate.h"
#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"
#include "runtime/memory_column_index.h"

namespace kush::compile::proxy {

namespace {
constexpr std::string_view StructName(
    "kush::runtime::MemoryColumnIndex::MemoryColumnIndex");
constexpr std::string_view InitFnName("kush::runtime::MemoryColumnIndex::Init");
constexpr std::string_view AllocateNewPageFnName(
    "kush::runtime::MemoryColumnIndex::AllocateNewPage");
constexpr std::string_view ResizeFnName(
    "kush::runtime::MemoryColumnIndex::Resize");
constexpr std::string_view FreeFnName("kush::runtime::MemoryColumnIndex::Free");
constexpr std::string_view GetPayloadFnName(
    "kush::runtime::MemoryColumnIndex::GetPayload");
constexpr std::string_view GetEntryFnName(
    "kush::runtime::MemoryColumnIndex::GetEntry");
}  // namespace

MemoryColumnIndexEntry::MemoryColumnIndexEntry(khir::ProgramBuilder& program,
                                               khir::Value v)
    : program_(program), value_(v) {}

MemoryColumnIndexEntry::Parts MemoryColumnIndexEntry::Get() {
  auto i64 = program_.LoadI64(value_);

  auto salt = program_.RShiftI64(i64, 48);
  auto salt_typed = program_.I16TruncI64(salt);

  auto block_offset_unmasked = program_.RShiftI64(i64, 32);
  auto block_offset_typed = program_.I16TruncI64(block_offset_unmasked);

  auto block_idx = program_.I32TruncI64(i64);

  return Parts{.salt = Int16(program_, salt_typed),
               .block_offset = Int16(program_, block_offset_typed),
               .block_idx = Int32(program_, block_idx)};
}

void MemoryColumnIndexEntry::Set(Int16 salt, Int16 offset, Int32 idx) {
  auto salt_zext = program_.I64ZextI16(salt.Get());
  auto salt_pos = program_.LShiftI64(salt_zext, 48);

  auto offset_zext = program_.I64ZextI16(offset.Get());
  auto offset_pos = program_.LShiftI64(offset_zext, 32);

  auto salt_offset = program_.OrI64(salt_pos, offset_pos);
  auto idx_pos = program_.I64ZextI32(idx.Get());
  auto i64 = program_.OrI64(salt_offset, idx_pos);

  program_.StoreI64(value_, i64);
}

khir::Type MemoryColumnIndexPayload::ConstructPayloadFormat(
    khir::ProgramBuilder& program, catalog::SqlType type) {
  std::vector<khir::Type> fields;

  // hash
  fields.push_back(program.I64Type());

  // key
  switch (type) {
    case catalog::SqlType::SMALLINT:
      fields.push_back(program.I16Type());
      break;
    case catalog::SqlType::INT:
    case catalog::SqlType::DATE:
      fields.push_back(program.I32Type());
      break;
    case catalog::SqlType::BIGINT:
      fields.push_back(program.I64Type());
      break;
    case catalog::SqlType::REAL:
      fields.push_back(program.F64Type());
      break;
    case catalog::SqlType::TEXT:
      fields.push_back(program.GetStructType(String::StringStructName));
      break;
    case catalog::SqlType::BOOLEAN:
      fields.push_back(program.I8Type());
      break;
  }

  // Column Index Bucket
  fields.push_back(program.GetStructType(ColumnIndexBucket::StructName));
  return program.StructType(fields);
}

khir::Value MemoryColumnIndexPayload::GetHashOffset(
    khir::ProgramBuilder& program, khir::Type type) {
  return program.PointerCast(
      program.StaticGEP(type, program.NullPtr(program.PointerType(type)),
                        {0, 0}),
      program.I64Type());
}

MemoryColumnIndexPayload::MemoryColumnIndexPayload(
    khir::ProgramBuilder& program, khir::Type t, catalog::SqlType kt,
    khir::Value v)
    : program_(program), type_(t), key_type_(kt), value_(v) {}

void MemoryColumnIndexPayload::Initialize(Int64 hash, const IRValue& key,
                                          khir::Value allocator) {
  program_.StoreI64(program_.StaticGEP(type_, value_, {0, 0}), hash.Get());

  {
    auto ptr = program_.StaticGEP(type_, value_, {0, 1});
    switch (key_type_) {
      case catalog::SqlType::SMALLINT:
        program_.StoreI16(ptr, key.Get());
        break;
      case catalog::SqlType::DATE:
      case catalog::SqlType::INT:
        program_.StoreI32(ptr, key.Get());
        break;
      case catalog::SqlType::BIGINT:
        program_.StoreI64(ptr, key.Get());
        break;
      case catalog::SqlType::REAL:
        program_.StoreF64(ptr, key.Get());
        break;
      case catalog::SqlType::TEXT:
        String(program_, ptr).Copy(dynamic_cast<const String&>(key));
        break;
      case catalog::SqlType::BOOLEAN:
        program_.StoreI8(ptr, program_.I8ZextI1(key.Get()));
        break;
    }
  }

  GetBucket().Init(allocator);
}

SQLValue MemoryColumnIndexPayload::GetKey() {
  auto ptr = program_.StaticGEP(type_, value_, {0, 1});
  switch (key_type_) {
    case catalog::SqlType::SMALLINT:
      return SQLValue(Int16(program_, program_.LoadI16(ptr)),
                      Bool(program_, false));
    case catalog::SqlType::DATE:
    case catalog::SqlType::INT:
      return SQLValue(Int32(program_, program_.LoadI32(ptr)),
                      Bool(program_, false));
    case catalog::SqlType::BIGINT:
      return SQLValue(Int64(program_, program_.LoadI64(ptr)),
                      Bool(program_, false));
    case catalog::SqlType::REAL:
      return SQLValue(Float64(program_, program_.LoadF64(ptr)),
                      Bool(program_, false));
    case catalog::SqlType::TEXT:
      return SQLValue(String(program_, ptr), Bool(program_, false));
    case catalog::SqlType::BOOLEAN:
      return SQLValue(Bool(program_, program_.LoadI8(ptr)),
                      Bool(program_, false));
  }
}

ColumnIndexBucket MemoryColumnIndexPayload::GetBucket() {
  return ColumnIndexBucket(program_, program_.StaticGEP(type_, value_, {0, 2}));
}

}  // namespace kush::compile::proxy