#pragma once

#include <functional>
#include <utility>
#include <vector>

#include "compile/proxy/aggregator.h"
#include "compile/proxy/struct.h"
#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

class AggregateHashTableEntry {
 public:
  AggregateHashTableEntry(khir::ProgramBuilder& program, khir::Value v);

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

class AggregateHashTablePayload {
 public:
  AggregateHashTablePayload(khir::ProgramBuilder& program,
                            const StructBuilder& format, khir::Value v);

  void Initialize(Int64 hash, const std::vector<SQLValue>& keys,
                  const std::vector<std::unique_ptr<Aggregator>>& aggregators);
  void Update(const std::vector<std::unique_ptr<Aggregator>>& aggregators);

  SQLValue GetKey(int key);

  // Gets the vector of <key values, aggregate values>
  std::vector<SQLValue> GetPayload(
      int num_keys,
      const std::vector<std::unique_ptr<Aggregator>>& aggregators);

  static StructBuilder ConstructPayloadFormat(
      khir::ProgramBuilder& program,
      const std::vector<std::pair<catalog::Type, bool>>& key_types,
      const std::vector<std::unique_ptr<Aggregator>>& aggregators);

  static khir::Value GetHashOffset(khir::ProgramBuilder& program,
                                   StructBuilder& format);

 private:
  khir::ProgramBuilder& program_;
  Struct content_;
};

class AggregateHashTable {
 public:
  AggregateHashTable(khir::ProgramBuilder& program,
                     std::vector<std::pair<catalog::Type, bool>> key_types,
                     std::vector<std::unique_ptr<Aggregator>> aggregators);
  void UpdateOrInsert(const std::vector<SQLValue>& keys);
  void ForEach(std::function<void(std::vector<SQLValue>)> handler);
  void Reset();

  static void ForwardDeclare(khir::ProgramBuilder& program);

 private:
  Int64 Hash(const std::vector<SQLValue>& keys);
  Int16 Salt(Int64 hash);
  AggregateHashTablePayload Insert(AggregateHashTableEntry& entry, Int64 hash,
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
  AggregateHashTablePayload GetPayload(Int32 block_idx, Int16 block_offset);
  AggregateHashTableEntry GetEntry(Int32 entry_idx);

  khir::ProgramBuilder& program_;
  std::vector<std::unique_ptr<Aggregator>> aggregators_;
  int num_keys_;
  StructBuilder payload_format_;
  khir::Value value_;
};

}  // namespace kush::compile::proxy