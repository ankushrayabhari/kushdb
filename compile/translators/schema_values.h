#pragma once

#include <memory>
#include <string>
#include <vector>

#include "compile/proxy/value/sql_value.h"
#include "khir/program_builder.h"
#include "plan/operator/operator_schema.h"

namespace kush::compile {

class SchemaValues {
 public:
  void PopulateWithNull(khir::ProgramBuilder& program,
                        const plan::OperatorSchema& op);
  void PopulateWithNotNull(khir::ProgramBuilder& program,
                           const plan::OperatorSchema& op);

  void ResetValues();
  void AddVariable(proxy::SQLValue value);

  const proxy::SQLValue& Value(int idx) const;
  const std::vector<proxy::SQLValue>& Values() const;
  std::vector<proxy::SQLValue>& Values();

  void SetValues(std::vector<proxy::SQLValue> values);
  void SetValue(int idx, proxy::SQLValue value);

 private:
  std::vector<proxy::SQLValue> values_;
};

}  // namespace kush::compile
