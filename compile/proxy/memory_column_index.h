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
  SQLValue GetKey();

  static khir::Type ConstructPayloadFormat(khir::ProgramBuilder& program,
                                           catalog::SqlType t);

  static khir::Value GetHashOffset(khir::ProgramBuilder& program, khir::Type t);

 private:
  khir::ProgramBuilder& program_;
  khir::Type type_;
  catalog::SqlType key_type_;
  khir::Value value_;
};

}  // namespace kush::compile::proxy