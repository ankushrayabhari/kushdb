#pragma once

#include <memory>

#include "catalog/sql_type.h"
#include "compile/proxy/column_index.h"
#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

template <catalog::SqlType S>
class MemoryColumnIndex : public ColumnIndex, public ColumnIndexBuilder {
 public:
  MemoryColumnIndex(khir::ProgramBuilder& program);
  MemoryColumnIndex(khir::ProgramBuilder& program, khir::Value v);
  virtual ~MemoryColumnIndex() = default;

  void Init() override;
  void Reset() override;
  void Insert(const IRValue& v, const Int32& tuple_idx) override;
  ColumnIndexBucket GetBucket(const IRValue& v) override;
  khir::Value Serialize() override;
  std::unique_ptr<ColumnIndex> Regenerate(khir::ProgramBuilder& program,
                                          void* value) override;

  static void ForwardDeclare(khir::ProgramBuilder& program);

 private:
  khir::ProgramBuilder& program_;
  khir::Value value_;
  khir::Value get_value_;
};

}  // namespace kush::compile::proxy