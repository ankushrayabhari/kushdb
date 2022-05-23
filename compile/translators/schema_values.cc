#include "compile/translators/schema_values.h"

#include <memory>
#include <string>
#include <vector>

#include "compile/proxy/value/ir_value.h"
#include "compile/proxy/value/sql_value.h"
#include "plan/operator/operator.h"
#include "util/vector_util.h"

namespace kush::compile {

void SchemaValues::PopulateWithNull(khir::ProgramBuilder& program,
                                    const plan::OperatorSchema& op) {
  ResetValues();
  const auto& schema = op.Columns();
  for (int i = 0; i < schema.size(); i++) {
    const auto& type = schema[i].Expr().Type();
    switch (type.type_id) {
      case catalog::TypeId::SMALLINT:
        AddVariable(proxy::SQLValue(proxy::Int16(program, 0),
                                    proxy::Bool(program, true)));
        break;
      case catalog::TypeId::INT:
        AddVariable(proxy::SQLValue(proxy::Int32(program, 0),
                                    proxy::Bool(program, true)));
        break;
      case catalog::TypeId::DATE:
        AddVariable(proxy::SQLValue(
            proxy::Date(program, runtime::Date::DateBuilder(2000, 1, 1)),
            proxy::Bool(program, true)));
        break;
      case catalog::TypeId::BIGINT:
        AddVariable(proxy::SQLValue(proxy::Int64(program, 0),
                                    proxy::Bool(program, true)));
        break;
      case catalog::TypeId::BOOLEAN:
        AddVariable(proxy::SQLValue(proxy::Bool(program, false),
                                    proxy::Bool(program, true)));
        break;
      case catalog::TypeId::REAL:
        AddVariable(proxy::SQLValue(proxy::Float64(program, 0),
                                    proxy::Bool(program, true)));
        break;
      case catalog::TypeId::TEXT:
        AddVariable(proxy::SQLValue(proxy::String::Global(program, ""),
                                    proxy::Bool(program, true)));
        break;
      case catalog::TypeId::ENUM:
        AddVariable(proxy::SQLValue(proxy::Enum(program, type.enum_id, -1),
                                    proxy::Bool(program, true)));
        break;
    }
  }
}

void SchemaValues::PopulateWithNotNull(khir::ProgramBuilder& program,
                                       const plan::OperatorSchema& op) {
  ResetValues();
  const auto& schema = op.Columns();
  for (int i = 0; i < schema.size(); i++) {
    const auto& type = schema[i].Expr().Type();
    switch (type.type_id) {
      case catalog::TypeId::SMALLINT:
        AddVariable(proxy::SQLValue(proxy::Int16(program, 0),
                                    proxy::Bool(program, false)));
        break;
      case catalog::TypeId::INT:
        AddVariable(proxy::SQLValue(proxy::Int32(program, 0),
                                    proxy::Bool(program, false)));
        break;
      case catalog::TypeId::DATE:
        AddVariable(proxy::SQLValue(
            proxy::Date(program, runtime::Date::DateBuilder(2000, 1, 1)),
            proxy::Bool(program, false)));
        break;
      case catalog::TypeId::BIGINT:
        AddVariable(proxy::SQLValue(proxy::Int64(program, 0),
                                    proxy::Bool(program, false)));
        break;
      case catalog::TypeId::BOOLEAN:
        AddVariable(proxy::SQLValue(proxy::Bool(program, false),
                                    proxy::Bool(program, false)));
        break;
      case catalog::TypeId::REAL:
        AddVariable(proxy::SQLValue(proxy::Float64(program, 0),
                                    proxy::Bool(program, false)));
        break;
      case catalog::TypeId::TEXT:
        AddVariable(proxy::SQLValue(proxy::String::Global(program, ""),
                                    proxy::Bool(program, false)));
        break;
      case catalog::TypeId::ENUM:
        AddVariable(proxy::SQLValue(proxy::Enum(program, type.enum_id, -1),
                                    proxy::Bool(program, false)));
        break;
    }
  }
}

void SchemaValues::ResetValues() { values_.clear(); }

void SchemaValues::AddVariable(proxy::SQLValue value) {
  values_.push_back(std::move(value));
}

const proxy::SQLValue& SchemaValues::Value(int idx) const {
  return values_[idx];
}

const std::vector<proxy::SQLValue>& SchemaValues::Values() const {
  return values_;
}

std::vector<proxy::SQLValue>& SchemaValues::Values() { return values_; }

void SchemaValues::SetValues(std::vector<proxy::SQLValue> values) {
  values_ = std::move(values);
}

void SchemaValues::SetValue(int idx, proxy::SQLValue value) {
  values_[idx] = std::move(value);
}

}  // namespace kush::compile
