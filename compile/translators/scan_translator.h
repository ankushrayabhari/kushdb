#pragma once

#include <functional>

#include "absl/container/flat_hash_map.h"

#include "compile/proxy/column_data.h"
#include "compile/proxy/value/ir_value.h"
#include "compile/proxy/value/sql_value.h"
#include "compile/translators/operator_translator.h"
#include "khir/program_builder.h"
#include "plan/scan_operator.h"

namespace kush::compile {

class ScanTranslator : public OperatorTranslator {
 public:
  ScanTranslator(const plan::ScanOperator& scan, khir::ProgramBuilder& program,
                 std::vector<std::unique_ptr<OperatorTranslator>> children);
  virtual ~ScanTranslator() = default;

  void Produce() override;
  void Consume(OperatorTranslator& src) override;

  void Open();
  void Produce(int col_idx, std::function<void(const proxy::Int32& tuple_idx,
                                               const proxy::SQLValue& value)>
                                handler);

  proxy::Int32 Size();
  proxy::SQLValue Get(int col_idx, proxy::Int32 i);
  void Reset();

 private:
  std::vector<std::unique_ptr<proxy::Iterable>> column_data_vars_;
  std::vector<std::unique_ptr<proxy::Iterable>> null_data_vars_;
  std::unique_ptr<proxy::Int32> size_var_;

  std::unique_ptr<proxy::Iterable> ConstructIterable(catalog::SqlType type,
                                                     std::string_view path);

  const plan::ScanOperator& scan_;
  khir::ProgramBuilder& program_;
};

}  // namespace kush::compile