#pragma once

#include <memory>

#include "catalog/sql_type.h"
#include "compile/proxy/column_index.h"
#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

template <catalog::TypeId S>
class DiskColumnIndex : public ColumnIndex {
 public:
  DiskColumnIndex(khir::ProgramBuilder& program, std::string_view path);
  DiskColumnIndex(khir::ProgramBuilder& program, std::string_view path,
                  khir::Value v);
  virtual ~DiskColumnIndex() = default;

  void Init() override;
  void Reset() override;
  ColumnIndexBucket GetBucket(const IRValue& v) override;
  khir::Value Serialize() override;
  std::unique_ptr<ColumnIndex> Regenerate(khir::ProgramBuilder& program,
                                          void* value) override;

  static void ForwardDeclare(khir::ProgramBuilder& program);

 private:
  khir::ProgramBuilder& program_;
  std::string path_;
  khir::Value path_value_;
  khir::Value value_;
  khir::Value get_value_;
};

}  // namespace kush::compile::proxy