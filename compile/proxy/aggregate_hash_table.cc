#include "compile/proxy/aggregate_hash_table.h"

#include <functional>
#include <utility>
#include <vector>

#include "compile/proxy/aggregator.h"
#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"
#include "runtime/aggregate_hash_table.h"

namespace kush::compile::proxy {

namespace {

constexpr std::string_view StructName(
    "kush::runtime::AggregateHashTable::AggregateHashTable");

constexpr std::string_view InitFnName(
    "kush::runtime::AggregateHashTable::Init");

constexpr std::string_view AllocateNewPageFnName(
    "kush::runtime::AggregateHashTable::AllocateNewPage");

constexpr std::string_view ResizeFnName(
    "kush::runtime::AggregateHashTable::Resize");

constexpr std::string_view FreeFnName(
    "kush::runtime::AggregateHashTable::Free");

}  // namespace

AggregateHashTableEntry::AggregateHashTableEntry(khir::ProgramBuilder& program,
                                                 khir::Value v)
    : program_(program), value_(v) {}

AggregateHashTableEntry::Parts AggregateHashTableEntry::Get() {
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

void AggregateHashTableEntry::Set(Int16 salt, Int16 offset, Int32 idx) {
  auto salt_zext = program_.I64ZextI16(salt.Get());
  auto salt_pos = program_.LShiftI64(salt_zext, 48);

  auto offset_zext = program_.I64ZextI16(offset.Get());
  auto offset_pos = program_.LShiftI64(offset_zext, 32);

  auto salt_offset = program_.OrI64(salt_pos, offset_pos);
  auto idx_pos = program_.I64ZextI32(idx.Get());
  auto i64 = program_.OrI64(salt_offset, idx_pos);

  program_.StoreI64(value_, i64);
}

AggregateHashTablePayload::AggregateHashTablePayload(
    khir::ProgramBuilder& program, const StructBuilder& format, khir::Value v)
    : program_(program), content_(program, format, v) {}

void AggregateHashTablePayload::Initialize(
    Int64 hash, const std::vector<SQLValue>& keys,
    const std::vector<std::unique_ptr<Aggregator>>& aggregators) {
  content_.Update(0, SQLValue(hash, Bool(program_, false)));

  for (int i = 0; i < keys.size(); i++) {
    content_.Update(i + 1, keys[i]);
  }

  for (const auto& agg : aggregators) {
    agg->Initialize(content_);
  }
}

void AggregateHashTablePayload::Update(
    const std::vector<std::unique_ptr<Aggregator>>& aggregators) {
  for (const auto& agg : aggregators) {
    agg->Update(content_);
  }
}

std::vector<SQLValue> AggregateHashTablePayload::GetPayload(
    int num_keys, const std::vector<std::unique_ptr<Aggregator>>& aggregators) {
  std::vector<SQLValue> output;

  for (int i = 0; i < num_keys; i++) {
    output.push_back(content_.Get(i + 1));
  }

  for (const auto& agg : aggregators) {
    output.push_back(agg->Get(content_));
  }

  return output;
}

StructBuilder AggregateHashTablePayload::ConstructPayloadFormat(
    khir::ProgramBuilder& program,
    const std::vector<std::pair<catalog::SqlType, bool>>& key_types,
    const std::vector<std::unique_ptr<Aggregator>>& aggregators) {
  proxy::StructBuilder format(program);

  // hash
  format.Add(catalog::SqlType::BIGINT, false);

  // keys
  for (auto [type, nullable] : key_types) {
    format.Add(type, nullable);
  }

  // agg state
  for (const auto& agg : aggregators) {
    agg->AddFields(format);
  }

  format.Build();
  return format;
}

khir::Value AggregateHashTablePayload::GetHashOffset(
    khir::ProgramBuilder& program, StructBuilder& format) {
  auto payload_type = format.Type();
  return program.PointerCast(
      program.ConstGEP(payload_type, program.NullPtr(payload_type), {0, 0}),
      program.I64Type());
}

AggregateHashTable::AggregateHashTable(
    khir::ProgramBuilder& program,
    std::vector<std::pair<catalog::SqlType, bool>> key_types,
    std::vector<std::unique_ptr<Aggregator>> aggregators)
    : program_(program),
      payload_format_(AggregateHashTablePayload::ConstructPayloadFormat(
          program, key_types, aggregators)),
      value_(program_.Global(
          false, true, program_.GetStructType(StructName),
          program_.ConstantStruct(
              program_.GetStructType(StructName),
              {
                  program.ConstI64(0),
                  program.ConstI64(0),
                  program.ConstI32(0),
                  program.ConstI32(0),
                  program.ConstI64(0),
                  program.NullPtr(program.PointerType(program.I64Type())),
                  program.NullPtr(program.PointerType(
                      program.PointerType(program.I8Type()))),
                  program.ConstI32(0),
                  program.ConstI32(0),
                  program.ConstI16(0),
              }))) {
  auto payload_type = payload_format_.Type();
  program_.Call(
      program_.GetFunction(InitFnName),
      {value_, program_.SizeOf(payload_type),
       AggregateHashTablePayload::GetHashOffset(program_, payload_format_)});
}

void AggregateHashTable::ForwardDeclare(khir::ProgramBuilder& program) {
  auto block_ptr = program.PointerType(program.I8Type());

  auto struct_type = program.StructType(
      {
          program.I64Type(),                       // payload_size
          program.I64Type(),                       // tuple_hash_offset
          program.I32Type(),                       // size
          program.I32Type(),                       // capacity
          program.I64Type(),                       // mask
          program.PointerType(program.I64Type()),  // entries
          program.PointerType(block_ptr),          // payload_block
          program.I32Type(),                       // payload_block_capacity
          program.I32Type(),                       // payload_block_size
          program.I16Type(),                       // last_payload_offset
      },
      StructName);
  auto struct_ptr = program.PointerType(struct_type);

  program.DeclareExternalFunction(
      InitFnName, program.VoidType(),
      {struct_ptr, program.I64Type(), program.I64Type()},
      reinterpret_cast<void*>(&runtime::AggregateHashTable::Init));

  program.DeclareExternalFunction(
      AllocateNewPageFnName, program.VoidType(), {struct_ptr},
      reinterpret_cast<void*>(&runtime::AggregateHashTable::AllocateNewPage));

  program.DeclareExternalFunction(
      ResizeFnName, program.VoidType(), {struct_ptr},
      reinterpret_cast<void*>(&runtime::AggregateHashTable::Resize));

  program.DeclareExternalFunction(
      FreeFnName, program.VoidType(), {struct_ptr},
      reinterpret_cast<void*>(&runtime::AggregateHashTable::Free));
}

}  // namespace kush::compile::proxy