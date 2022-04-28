#pragma once

#include <memory>

#include "catalog/sql_type.h"
#include "compile/proxy/column_index.h"
#include "compile/proxy/value/ir_value.h"
#include "compile/proxy/value/sql_value.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

class MemoryColumnIndexEntry {
 public:
  MemoryColumnIndexEntry(khir::ProgramBuilder& program, khir::Value v);

  struct Parts {
    Int16 salt;
    Int16 block_offset;
    Int32 block_idx;
  };
  Parts Get();

  void Set(Int16 salt, Int16 offset, Int32 idx);

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
};

class MemoryColumnIndexPayload {
 public:
  MemoryColumnIndexPayload(khir::ProgramBuilder& program, khir::Type t,
                           catalog::SqlType kt, khir::Value v);

  void Initialize(Int64 hash, const IRValue& key, khir::Value allocator);

  ColumnIndexBucket GetBucket();
  std::unique_ptr<IRValue> GetKey();

  static khir::Type ConstructPayloadFormat(khir::ProgramBuilder& program,
                                           catalog::SqlType t);

  static khir::Value GetHashOffset(khir::ProgramBuilder& program, khir::Type t);

 private:
  khir::ProgramBuilder& program_;
  khir::Type type_;
  catalog::SqlType key_type_;
  khir::Value value_;
};

class MemoryColumnIndex : public ColumnIndex, public ColumnIndexBuilder {
 public:
  MemoryColumnIndex(khir::ProgramBuilder& program, catalog::SqlType key_type);
  MemoryColumnIndex(khir::ProgramBuilder& program, catalog::SqlType key_type,
                    khir::Value value);

  void Init() override;
  void Insert(const IRValue& v, const Int32& tuple_idx) override;
  ColumnIndexBucket GetBucket(const IRValue& v) override;
  khir::Value Serialize() override;
  std::unique_ptr<ColumnIndex> Regenerate(khir::ProgramBuilder& program,
                                          void* value) override;
  void Reset() override;

  static void ForwardDeclare(khir::ProgramBuilder& program);

 private:
  Int64 Hash(const IRValue& key);
  Int16 Salt(Int64 hash);
  MemoryColumnIndexPayload Insert(MemoryColumnIndexEntry& entry, Int64 hash,
                                  Int16 salt);

  void Resize();
  void AllocateNewPage();

  void SetSize(Int32 size);
  Int32 Size();
  Int64 Mask();
  Int32 Capacity();
  Int16 PayloadSize();
  Int64 PayloadHashOffset();
  Int32 PayloadBlocksSize();
  void SetPayloadBlocksOffset(Int16 s);
  Int16 PayloadBlocksOffset();
  MemoryColumnIndexPayload GetPayload(Int32 block_idx, Int16 block_offset);
  MemoryColumnIndexEntry GetEntry(Int32 entry_idx);
  khir::Value Allocator();

  khir::ProgramBuilder& program_;
  catalog::SqlType key_type_;
  khir::Type payload_format_;
  khir::Value value_;
  khir::Value get_value_;
};

}  // namespace kush::compile::proxy