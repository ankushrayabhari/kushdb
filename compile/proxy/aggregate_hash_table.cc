#include "compile/proxy/aggregate_hash_table.h"

#include <functional>
#include <utility>
#include <vector>

#include "compile/proxy/aggregator.h"
#include "compile/proxy/control_flow/if.h"
#include "compile/proxy/control_flow/loop.h"
#include "compile/proxy/evaluate.h"
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
constexpr std::string_view GetPayloadFnName(
    "kush::runtime::AggregateHashTable::GetPayload");
constexpr std::string_view GetEntryFnName(
    "kush::runtime::AggregateHashTable::GetEntry");
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

SQLValue AggregateHashTablePayload::GetKey(int field) {
  return content_.Get(field + 1);
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
  auto payload_type = program.PointerType(format.Type());
  return program.PointerCast(
      program.ConstGEP(payload_type, program.NullPtr(payload_type), {0, 0}),
      program.I64Type());
}

AggregateHashTable::AggregateHashTable(
    khir::ProgramBuilder& program,
    std::vector<std::pair<catalog::SqlType, bool>> key_types,
    std::vector<std::unique_ptr<Aggregator>> aggregators)
    : program_(program),
      aggregators_(std::move(aggregators)),
      payload_format_(AggregateHashTablePayload::ConstructPayloadFormat(
          program, key_types, aggregators_)),
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

Bool CheckEq(khir::ProgramBuilder& program, AggregateHashTablePayload& payload,
             const std::vector<SQLValue>& keys, int idx = 0) {
  if (idx == keys.size()) {
    return Bool(program, true);
  }

  auto value = payload.GetKey(idx);
  return Ternary(
      program, Equal(value, keys[idx]),
      [&] { return CheckEq(program, payload, keys, idx + 1); },
      [&]() { return Bool(program, false); });
}

void AggregateHashTable::UpdateOrInsert(const std::vector<SQLValue>& keys) {
  // entry_idx = hash & mask
  // while !found
  //     get the hash entry at entry_idx
  //     if empty:
  //        payload = hash_insert(entry, salt)
  //        insert_handler(payload)
  //        found = true
  //     else if salt is equal and key_eq():
  //        update()
  //        found = true
  //     else:
  //         entry_idx++
  //         if (entry_idx == capacity):
  //             entry_idx = 0

  auto hash = Hash(keys);
  auto salt = Salt(hash);

  auto idx = hash & Mask();
  auto capacity = Capacity();
  Loop(
      program_,
      [&](auto& loop) {
        loop.AddLoopVariable(Bool(program_, false));
        loop.AddLoopVariable(idx);
      },
      [&](auto& loop) {
        auto found = loop.template GetLoopVariable<Bool>(0);
        return found;
      },
      [&](auto& loop) {
        auto idx = loop.template GetLoopVariable<Int32>(1);
        auto entry = GetEntry(idx);
        auto entry_parts = entry.Get();

        If(program_, entry_parts.block_idx == 0, [&] {
          auto payload = Insert(entry, hash, salt);
          payload.Initialize(hash, keys, aggregators_);
          loop.Continue(Bool(program_, true), idx);
        });

        If(program_, entry_parts.salt == salt, [&] {
          auto payload =
              GetPayload(entry_parts.block_idx, entry_parts.block_offset);
          If(program_, CheckEq(program_, payload, keys), [&]() {
            payload.Update(aggregators_);
            loop.Continue(Bool(program_, true), idx);
          });
        });

        auto next_idx = idx + 1;
        auto next_idx_mod = Ternary(
            program_, next_idx < capacity, [&]() { return next_idx; },
            [&]() { return Int32(program_, 0); });
        return loop.Continue(Bool(program_, false), next_idx_mod);
      });
}

AggregateHashTablePayload AggregateHashTable::Insert(
    AggregateHashTableEntry& entry, Int64 hash, Int16 salt) {
  auto size = Size();
  auto capacity = Capacity();
  If(
      program_, size + 1 <= capacity, [&]() { Resize(); },
      [&]() {
        If(program_,
           Float64(program_, size) >
               Float64(program_, capacity) /
                   Float64(program_, runtime::AggregateHashTable::LOAD_FACTOR),
           [&]() { Resize(); });
      });

  If(program_,
     PayloadBlocksOffset().Zext() + PayloadSize() >=
         Int64(program_, runtime::AggregateHashTable::BLOCK_SIZE),
     [&]() { AllocateNewPage(); });

  auto block_idx = PayloadBlocksSize();
  auto offset = PayloadBlocksOffset();
  entry.Set(salt, offset, block_idx);

  return GetPayload(block_idx, offset);
}

Int64 AggregateHashTable::Hash(const std::vector<SQLValue>& keys) {
  Int64 hash(program_, 0);

  Int64 magic(program_, 0x9e3779b97f4a7c15);

  for (auto& k : keys) {
    auto key_hash = proxy::Ternary(
        program_, k.IsNull(), [&]() { return Int64(program_, 0); },
        [&]() { return k.Get().Hash(); });

    hash = hash ^ (key_hash + magic + (hash << 12) + (hash >> 4));
  }

  return hash;
}

Int16 AggregateHashTable::Salt(Int64 hash) {
  return Int16(program_, program_.I16TruncI64((hash >> 48).Get()));
}

void AggregateHashTable::Reset() {
  program_.Call(program_.GetFunction(FreeFnName), {value_});
}

void AggregateHashTable::AllocateNewPage() {
  program_.Call(program_.GetFunction(AllocateNewPageFnName), {value_});
}

void AggregateHashTable::Resize() {
  program_.Call(program_.GetFunction(ResizeFnName), {value_});
}

Int64 AggregateHashTable::PayloadSize() {
  auto type = program_.PointerType(payload_format_.Type());
  return Int64(program_,
               program_.LoadI64(program_.ConstGEP(type, value_, {0, 0})));
}

Int64 AggregateHashTable::PayloadHashOffset() {
  auto type = program_.PointerType(payload_format_.Type());
  return Int64(program_,
               program_.LoadI64(program_.ConstGEP(type, value_, {0, 1})));
}

Int32 AggregateHashTable::Size() {
  auto type = program_.PointerType(payload_format_.Type());
  return Int32(program_,
               program_.LoadI32(program_.ConstGEP(type, value_, {0, 2})));
}

Int32 AggregateHashTable::Capacity() {
  auto type = program_.PointerType(payload_format_.Type());
  return Int32(program_,
               program_.LoadI32(program_.ConstGEP(type, value_, {0, 3})));
}

Int64 AggregateHashTable::Mask() {
  auto type = program_.PointerType(payload_format_.Type());
  return Int64(program_,
               program_.LoadI64(program_.ConstGEP(type, value_, {0, 4})));
}

Int32 AggregateHashTable::PayloadBlocksSize() {
  auto type = program_.PointerType(payload_format_.Type());
  return Int32(program_,
               program_.LoadI32(program_.ConstGEP(type, value_, {0, 8})));
}

Int16 AggregateHashTable::PayloadBlocksOffset() {
  auto type = program_.PointerType(payload_format_.Type());
  return Int16(program_,
               program_.LoadI16(program_.ConstGEP(type, value_, {0, 9})));
}

AggregateHashTableEntry AggregateHashTable::GetEntry(Int32 entry_idx) {
  return AggregateHashTableEntry(
      program_, program_.Call(program_.GetFunction(GetEntryFnName),
                              {value_, entry_idx.Get()}));
}

AggregateHashTablePayload AggregateHashTable::GetPayload(Int32 block_idx,
                                                         Int16 block_offset) {
  auto type = program_.PointerType(payload_format_.Type());
  return AggregateHashTablePayload(
      program_, payload_format_,
      program_.PointerCast(
          program_.Call(program_.GetFunction(GetPayloadFnName),
                        {value_, block_idx.Get(), block_offset.Get()}),
          type));
}

void AggregateHashTable::ForwardDeclare(khir::ProgramBuilder& program) {
  auto block_ptr = program.PointerType(program.I8Type());

  auto struct_type = program.StructType(
      {
          program.I64Type(),                       // payload_size
          program.I64Type(),                       // payload_hash_offset
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

  program.DeclareExternalFunction(
      GetEntryFnName, program.PointerType(program.I64Type()),
      {struct_ptr, program.I32Type()},
      reinterpret_cast<void*>(&runtime::AggregateHashTable::Free));

  program.DeclareExternalFunction(
      GetPayloadFnName, program.PointerType(program.I8Type()),
      {struct_ptr, program.I32Type(), program.I16Type()},
      reinterpret_cast<void*>(&runtime::AggregateHashTable::Free));
}

}  // namespace kush::compile::proxy