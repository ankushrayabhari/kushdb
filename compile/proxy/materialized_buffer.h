#pragma once

#include <memory>
#include <vector>

#include "compile/proxy/column_data.h"
#include "compile/proxy/value/ir_value.h"
#include "compile/proxy/value/sql_value.h"
#include "compile/proxy/vector.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

/*
 * A materialized buffer is either:
 * - a base table on disk (collection of proxy::ColumnData)
 * - a proxy::Vector
 *
 * This is a wrapper object mainly for use in the SkinnerJoinTranslators as
 * this allows for easy regeneration in a new ProgramBuidler.
 */

class MaterializedBuffer {
 public:
  virtual ~MaterializedBuffer() = default;

  virtual void Init() = 0;
  virtual void Reset() = 0;
  virtual Int32 Size() = 0;
  virtual std::vector<SQLValue> operator[](Int32 i) = 0;
  virtual SQLValue Get(Int32 i, int col) = 0;

  // Serializes the data needed to regenerate a reference to this buffer in a
  // different program builder.
  // Returns an i8* typed Value
  virtual khir::Value Serialize() = 0;

  // Regenerates a reference to the buffer inside program. value's content is
  // the same as the one returned by the above Serialize method.
  virtual std::unique_ptr<MaterializedBuffer> Regenerate(
      khir::ProgramBuilder& program, void* value) = 0;
};

class DiskMaterializedBuffer : public MaterializedBuffer {
 public:
  DiskMaterializedBuffer(khir::ProgramBuilder& program,
                         std::vector<std::unique_ptr<Iterable>> column_data,
                         std::vector<std::unique_ptr<Iterable>> null_data);
  ~DiskMaterializedBuffer() = default;

  void Init() override;
  void Reset() override;
  Int32 Size() override;
  std::vector<SQLValue> operator[](Int32 i) override;
  SQLValue Get(Int32 i, int col) override;
  khir::Value Serialize() override;
  std::unique_ptr<MaterializedBuffer> Regenerate(khir::ProgramBuilder& program,
                                                 void* value) override;
  void Scan(int col_idx, std::function<void(Int32, SQLValue)> handler);

 private:
  khir::ProgramBuilder& program_;
  std::vector<std::unique_ptr<Iterable>> column_data_;
  std::vector<std::unique_ptr<Iterable>> null_data_;
};

class MemoryMaterializedBuffer : public MaterializedBuffer {
 public:
  MemoryMaterializedBuffer(khir::ProgramBuilder& program,
                           std::unique_ptr<StructBuilder> vector_content,
                           Vector vector);
  ~MemoryMaterializedBuffer() = default;

  void Init() override;
  void Reset() override;
  Int32 Size() override;
  std::vector<SQLValue> operator[](Int32 i) override;
  SQLValue Get(Int32 i, int col) override;
  khir::Value Serialize() override;
  std::unique_ptr<MaterializedBuffer> Regenerate(khir::ProgramBuilder& program,
                                                 void* value) override;

 private:
  khir::ProgramBuilder& program_;
  std::unique_ptr<StructBuilder> vector_content_;
  proxy::Vector vector_;
};

}  // namespace kush::compile::proxy
