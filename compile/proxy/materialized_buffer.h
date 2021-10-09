#pragma once

#include <memory>
#include <vector>

#include "compile/proxy/column_data.h"
#include "compile/proxy/value/ir_value.h"
#include "compile/proxy/value/sql_value.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

/*
 * A materialized buffer is either:
 * - a base table on disk (collection of proxy::ColumnData)
 * - a proxy::Vector
 *
 * This is a wrapper object mainly for use in the SkinnerJoinTranslators as
 * this allows for easy duplication in a new ProgramBuidler.
 */

class MaterializedBuffer {
 public:
  virtual ~MaterializedBuffer() = default;

  virtual void Init() = 0;
  virtual void Reset() = 0;

  virtual Int32 Size() = 0;
  virtual std::vector<SQLValue> operator[](Int32 i) = 0;
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

 private:
  khir::ProgramBuilder& program_;
  std::vector<std::unique_ptr<Iterable>> column_data_;
  std::vector<std::unique_ptr<Iterable>> null_data_;
  std::unique_ptr<Int32> size_var_;
};

}  // namespace kush::compile::proxy
