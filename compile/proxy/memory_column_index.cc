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

Bool CheckEq(const catalog::Type& type, const IRValue& lhs,
             const IRValue& rhs) {
  switch (type.type_id) {
    case catalog::TypeId::BOOLEAN: {
      auto& lhs_v = static_cast<const Bool&>(lhs);
      auto& rhs_v = static_cast<const Bool&>(rhs);
      return lhs_v == rhs_v;
    }

    case catalog::TypeId::SMALLINT: {
      auto& lhs_v = static_cast<const Int16&>(lhs);
      auto& rhs_v = static_cast<const Int16&>(rhs);
      return lhs_v == rhs_v;
    }

    case catalog::TypeId::INT: {
      auto& lhs_v = static_cast<const Int32&>(lhs);
      auto& rhs_v = static_cast<const Int32&>(rhs);
      return lhs_v == rhs_v;
    }

    case catalog::TypeId::DATE: {
      auto& lhs_v = static_cast<const Date&>(lhs);
      auto& rhs_v = static_cast<const Date&>(rhs);
      return lhs_v == rhs_v;
    }

    case catalog::TypeId::BIGINT: {
      auto& lhs_v = static_cast<const Int64&>(lhs);
      auto& rhs_v = static_cast<const Int64&>(rhs);
      return lhs_v == rhs_v;
    }

    case catalog::TypeId::REAL: {
      auto& lhs_v = static_cast<const Float64&>(lhs);
      auto& rhs_v = static_cast<const Float64&>(rhs);
      return lhs_v == rhs_v;
    }

    case catalog::TypeId::TEXT: {
      auto& lhs_v = static_cast<const String&>(lhs);
      auto& rhs_v = static_cast<const String&>(rhs);
      return lhs_v == rhs_v;
    }
  }
}

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
    khir::ProgramBuilder& program, const catalog::Type& type) {
  std::vector<khir::Type> fields;

  // hash
  fields.push_back(program.I64Type());

  // key
  switch (type.type_id) {
    case catalog::TypeId::SMALLINT:
      fields.push_back(program.I16Type());
      break;
    case catalog::TypeId::INT:
    case catalog::TypeId::DATE:
      fields.push_back(program.I32Type());
      break;
    case catalog::TypeId::BIGINT:
      fields.push_back(program.I64Type());
      break;
    case catalog::TypeId::REAL:
      fields.push_back(program.F64Type());
      break;
    case catalog::TypeId::TEXT:
      fields.push_back(program.GetStructType(String::StringStructName));
      break;
    case catalog::TypeId::BOOLEAN:
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
    khir::ProgramBuilder& program, khir::Type t, const catalog::Type& kt,
    khir::Value v)
    : program_(program), type_(t), key_type_(kt), value_(v) {}

void MemoryColumnIndexPayload::Initialize(Int64 hash, const IRValue& key,
                                          khir::Value allocator) {
  program_.StoreI64(program_.StaticGEP(type_, value_, {0, 0}), hash.Get());

  {
    auto ptr = program_.StaticGEP(type_, value_, {0, 1});
    switch (key_type_.type_id) {
      case catalog::TypeId::SMALLINT:
        program_.StoreI16(ptr, key.Get());
        break;
      case catalog::TypeId::DATE:
      case catalog::TypeId::INT:
        program_.StoreI32(ptr, key.Get());
        break;
      case catalog::TypeId::BIGINT:
        program_.StoreI64(ptr, key.Get());
        break;
      case catalog::TypeId::REAL:
        program_.StoreF64(ptr, key.Get());
        break;
      case catalog::TypeId::TEXT:
        String(program_, ptr).Copy(dynamic_cast<const String&>(key));
        break;
      case catalog::TypeId::BOOLEAN:
        program_.StoreI8(ptr, program_.I8ZextI1(key.Get()));
        break;
    }
  }

  GetBucket().Init(allocator);
}

std::unique_ptr<IRValue> MemoryColumnIndexPayload::GetKey() {
  auto ptr = program_.StaticGEP(type_, value_, {0, 1});
  switch (key_type_.type_id) {
    case catalog::TypeId::SMALLINT:
      return Int16(program_, program_.LoadI16(ptr)).ToPointer();
    case catalog::TypeId::DATE:
    case catalog::TypeId::INT:
      return Int32(program_, program_.LoadI32(ptr)).ToPointer();
    case catalog::TypeId::BIGINT:
      return Int64(program_, program_.LoadI64(ptr)).ToPointer();
    case catalog::TypeId::REAL:
      return Float64(program_, program_.LoadF64(ptr)).ToPointer();
    case catalog::TypeId::TEXT:
      return String(program_, ptr).ToPointer();
    case catalog::TypeId::BOOLEAN:
      return Bool(program_, program_.LoadI8(ptr)).ToPointer();
  }
}

ColumnIndexBucket MemoryColumnIndexPayload::GetBucket() {
  return ColumnIndexBucket(program_, program_.StaticGEP(type_, value_, {0, 2}));
}

MemoryColumnIndex::MemoryColumnIndex(khir::ProgramBuilder& program,
                                     const catalog::Type& key_type)
    : program_(program),
      key_type_(key_type),
      payload_format_(
          MemoryColumnIndexPayload::ConstructPayloadFormat(program_, key_type)),
      value_(program_.Global(
          program_.GetStructType(StructName),
          program_.ConstantStruct(
              program_.GetStructType(StructName),
              {
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
                  program.ConstI16(0),
                  program.NullPtr(program.PointerType(program.I8Type())),
              }))),
      get_value_(program.Global(
          program.GetStructType(ColumnIndexBucket::StructName),
          program.ConstantStruct(
              program.GetStructType(ColumnIndexBucket::StructName),
              {program.NullPtr(program.PointerType(program.I32Type())),
               program.ConstI32(0), program.ConstI32(0),
               program.NullPtr(program.PointerType(program.I8Type()))}))) {}

MemoryColumnIndex::MemoryColumnIndex(khir::ProgramBuilder& program,
                                     const catalog::Type& key_type,
                                     khir::Value value)
    : program_(program),
      key_type_(key_type),
      payload_format_(
          MemoryColumnIndexPayload::ConstructPayloadFormat(program_, key_type)),
      value_(value),
      get_value_(program.Global(
          program.GetStructType(ColumnIndexBucket::StructName),
          program.ConstantStruct(
              program.GetStructType(ColumnIndexBucket::StructName),
              {program.NullPtr(program.PointerType(program.I32Type())),
               program.ConstI32(0), program.ConstI32(0),
               program.NullPtr(program.PointerType(program.I8Type()))}))) {}

khir::Value MemoryColumnIndex::Serialize() {
  return program_.PointerCast(value_, program_.PointerType(program_.I8Type()));
}

std::unique_ptr<ColumnIndex> MemoryColumnIndex::Regenerate(
    khir::ProgramBuilder& program, void* value) {
  return std::make_unique<MemoryColumnIndex>(
      program, key_type_,
      program.PointerCast(
          program.ConstPtr(value),
          program.PointerType(program.GetStructType(StructName))));
}

void MemoryColumnIndex::Init() {
  program_.Call(
      program_.GetFunction(InitFnName),
      {value_, program_.I16TruncI64(program_.SizeOf(payload_format_)),
       MemoryColumnIndexPayload::GetHashOffset(program_, payload_format_)});
}

ColumnIndexBucket MemoryColumnIndex::GetBucket(const IRValue& key) {
  auto hash = Hash(key);
  auto salt = Salt(hash);

  auto mask = Mask();
  auto idx = hash & mask;
  auto capacity = Capacity();

  ColumnIndexBucket dest(program_, get_value_);
  Loop(
      program_,
      [&](auto& loop) {
        loop.AddLoopVariable(Bool(program_, true));
        loop.AddLoopVariable(idx);
      },
      [&](auto& loop) {
        auto not_found = loop.template GetLoopVariable<Bool>(0);
        return not_found;
      },
      [&](auto& loop) {
        auto idx = loop.template GetLoopVariable<Int64>(1);
        auto entry = GetEntry(Int32(program_, program_.I32TruncI64(idx.Get())));
        auto entry_parts = entry.Get();

        If(program_, entry_parts.block_idx == 0, [&] {
          dest.Copy(ColumnIndexBucket(
              program_,
              program_.PointerCast(program_.ConstPtr(&runtime::EMPTY),
                                   program_.PointerType(program_.GetStructType(
                                       ColumnIndexBucket::StructName)))));
          loop.Continue(Bool(program_, false), Int64(program_, 0));
        });

        If(program_, entry_parts.salt == salt, [&] {
          auto payload =
              GetPayload(entry_parts.block_idx, entry_parts.block_offset);
          auto payload_key = payload.GetKey();

          If(program_, CheckEq(key_type_, *payload_key, key), [&]() {
            dest.Copy(payload.GetBucket());
            loop.Continue(Bool(program_, false), Int64(program_, 0));
          });
        });

        return loop.Continue(Bool(program_, true), (idx + 1) & mask);
      });

  return dest;
}

void MemoryColumnIndex::Insert(const IRValue& key, const Int32& tuple_idx) {
  auto hash = Hash(key);
  auto salt = Salt(hash);

  auto mask = Mask();
  auto idx = hash & mask;
  auto capacity = Capacity();

  Loop(
      program_,
      [&](auto& loop) {
        loop.AddLoopVariable(Bool(program_, true));
        loop.AddLoopVariable(idx);
      },
      [&](auto& loop) {
        auto not_found = loop.template GetLoopVariable<Bool>(0);
        return not_found;
      },
      [&](auto& loop) {
        auto idx = loop.template GetLoopVariable<Int64>(1);
        auto entry = GetEntry(Int32(program_, program_.I32TruncI64(idx.Get())));
        auto entry_parts = entry.Get();

        If(program_, entry_parts.block_idx == 0, [&] {
          auto payload = Insert(entry, hash, salt);
          payload.Initialize(hash, key, Allocator());
          payload.GetBucket().PushBack(tuple_idx);
          loop.Continue(Bool(program_, false), Int64(program_, 0));
        });

        If(program_, entry_parts.salt == salt, [&] {
          auto payload =
              GetPayload(entry_parts.block_idx, entry_parts.block_offset);
          auto payload_key = payload.GetKey();
          If(program_, CheckEq(key_type_, *payload_key, key), [&]() {
            payload.GetBucket().PushBack(tuple_idx);
            loop.Continue(Bool(program_, false), Int64(program_, 0));
          });
        });

        return loop.Continue(Bool(program_, true), (idx + 1) & mask);
      });

  {  // Ensure we have an empty slot for future entries
    auto size = Size();
    auto capacity = Capacity();
    If(
        program_, size == capacity, [&]() { Resize(); },
        [&]() {
          If(program_,
             Float64(program_, size) >
                 Float64(program_, capacity) /
                     Float64(program_, runtime::MemoryColumnIndex::LOAD_FACTOR),
             [&]() { Resize(); });
        });
  }
}

MemoryColumnIndexPayload MemoryColumnIndex::Insert(
    MemoryColumnIndexEntry& entry, Int64 hash, Int16 salt) {
  SetSize(Size() + 1);

  // Allocate a new page if we have no more space left on the last one
  auto payload_size = PayloadSize();
  If(program_,
     PayloadBlocksOffset() + payload_size >
         Int16(program_, runtime::MemoryColumnIndex::BLOCK_SIZE),
     [&]() { AllocateNewPage(); });

  // last page = size - 1
  auto block_idx = PayloadBlocksSize() - 1;
  auto offset = PayloadBlocksOffset();
  SetPayloadBlocksOffset(offset + payload_size);

  entry.Set(salt, offset, block_idx);
  return GetPayload(block_idx, offset);
}

Int64 MemoryColumnIndex::Hash(const IRValue& key) {
  Int64 hash(program_, 0);
  Int64 magic(program_, 0xc6a4a7935bd1e995);
  Int64 offset(program_, 0xe6546b64);
  uint8_t r = 47;

  auto key_hash = key.Hash();
  key_hash = key_hash * magic;
  key_hash = key_hash ^ (key_hash >> r);
  key_hash = key_hash * magic;
  hash = hash ^ key_hash;
  hash = hash * magic;
  hash = hash + offset;
  return hash;
}

Int16 MemoryColumnIndex::Salt(Int64 hash) {
  return Int16(program_, program_.I16TruncI64((hash >> 48).Get()));
}

void MemoryColumnIndex::Reset() {
  program_.Call(program_.GetFunction(FreeFnName), {value_});
}

void MemoryColumnIndex::AllocateNewPage() {
  program_.Call(program_.GetFunction(AllocateNewPageFnName), {value_});
}

void MemoryColumnIndex::Resize() {
  program_.Call(program_.GetFunction(ResizeFnName), {value_});
}

Int64 MemoryColumnIndex::PayloadHashOffset() {
  return Int64(program_,
               program_.LoadI64(program_.StaticGEP(
                   program_.GetStructType(StructName), value_, {0, 0})));
}

void MemoryColumnIndex::SetSize(Int32 s) {
  program_.StoreI32(
      program_.StaticGEP(program_.GetStructType(StructName), value_, {0, 1}),
      s.Get());
}

Int32 MemoryColumnIndex::Size() {
  return Int32(program_,
               program_.LoadI32(program_.StaticGEP(
                   program_.GetStructType(StructName), value_, {0, 1})));
}

Int32 MemoryColumnIndex::Capacity() {
  return Int32(program_,
               program_.LoadI32(program_.StaticGEP(
                   program_.GetStructType(StructName), value_, {0, 2})));
}

Int64 MemoryColumnIndex::Mask() {
  return Int64(program_,
               program_.LoadI64(program_.StaticGEP(
                   program_.GetStructType(StructName), value_, {0, 3})));
}

Int32 MemoryColumnIndex::PayloadBlocksSize() {
  return Int32(program_,
               program_.LoadI32(program_.StaticGEP(
                   program_.GetStructType(StructName), value_, {0, 7})));
}

void MemoryColumnIndex::SetPayloadBlocksOffset(Int16 s) {
  program_.StoreI16(
      program_.StaticGEP(program_.GetStructType(StructName), value_, {0, 8}),
      s.Get());
}

Int16 MemoryColumnIndex::PayloadBlocksOffset() {
  return Int16(program_,
               program_.LoadI16(program_.StaticGEP(
                   program_.GetStructType(StructName), value_, {0, 8})));
}

Int16 MemoryColumnIndex::PayloadSize() {
  return Int16(program_,
               program_.LoadI64(program_.StaticGEP(
                   program_.GetStructType(StructName), value_, {0, 9})));
}

MemoryColumnIndexEntry MemoryColumnIndex::GetEntry(Int32 entry_idx) {
  return MemoryColumnIndexEntry(
      program_, program_.Call(program_.GetFunction(GetEntryFnName),
                              {value_, entry_idx.Get()}));
}

MemoryColumnIndexPayload MemoryColumnIndex::GetPayload(Int32 block_idx,
                                                       Int16 block_offset) {
  auto type = program_.PointerType(payload_format_);
  return MemoryColumnIndexPayload(
      program_, payload_format_, key_type_,
      program_.PointerCast(
          program_.Call(program_.GetFunction(GetPayloadFnName),
                        {value_, block_idx.Get(), block_offset.Zext().Get()}),
          type));
}

khir::Value MemoryColumnIndex::Allocator() {
  return program_.LoadPtr(
      program_.StaticGEP(program_.GetStructType(StructName), value_, {0, 10}));
}

void MemoryColumnIndex::ForwardDeclare(khir::ProgramBuilder& program) {
  auto block_ptr = program.PointerType(program.I8Type());

  auto struct_type = program.StructType(
      {
          program.I64Type(),                       // payload_hash_offset
          program.I32Type(),                       // size
          program.I32Type(),                       // capacity
          program.I64Type(),                       // mask
          program.PointerType(program.I64Type()),  // entries
          program.PointerType(block_ptr),          // payload_block
          program.I32Type(),                       // payload_block_capacity
          program.I32Type(),                       // payload_block_size
          program.I16Type(),                       // last_payload_offset
          program.I16Type(),                       // payload_size
          program.PointerType(program.I8Type()),   // allocator
      },
      StructName);
  auto struct_ptr = program.PointerType(struct_type);

  program.DeclareExternalFunction(
      InitFnName, program.VoidType(),
      {struct_ptr, program.I16Type(), program.I64Type()},
      reinterpret_cast<void*>(&runtime::MemoryColumnIndex::Init));

  program.DeclareExternalFunction(
      AllocateNewPageFnName, program.VoidType(), {struct_ptr},
      reinterpret_cast<void*>(&runtime::MemoryColumnIndex::AllocateNewPage));

  program.DeclareExternalFunction(
      ResizeFnName, program.VoidType(), {struct_ptr},
      reinterpret_cast<void*>(&runtime::MemoryColumnIndex::Resize));

  program.DeclareExternalFunction(
      FreeFnName, program.VoidType(), {struct_ptr},
      reinterpret_cast<void*>(&runtime::MemoryColumnIndex::Free));

  program.DeclareExternalFunction(
      GetEntryFnName, program.PointerType(program.I64Type()),
      {struct_ptr, program.I32Type()},
      reinterpret_cast<void*>(&runtime::MemoryColumnIndex::GetEntry));

  program.DeclareExternalFunction(
      GetPayloadFnName, program.PointerType(program.I8Type()),
      {struct_ptr, program.I32Type(), program.I64Type()},
      reinterpret_cast<void*>(&runtime::MemoryColumnIndex::GetPayload));
}

}  // namespace kush::compile::proxy