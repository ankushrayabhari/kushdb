#include "compile/proxy/aggregator.h"

#include <vector>

#include "catalog/sql_type.h"
#include "compile/proxy/aggregator.h"
#include "compile/proxy/control_flow/if.h"
#include "compile/proxy/struct.h"
#include "khir/program_builder.h"
#include "plan/expression/aggregate_expression.h"

namespace kush::compile::proxy {

SumAggregator::SumAggregator(
    khir::ProgramBuilder& program,
    util::Visitor<plan::ImmutableExpressionVisitor, const plan::Expression&,
                  SQLValue>& expr_translator,
    const kush::plan::AggregateExpression& agg)
    : program_(program),
      expr_translator_(expr_translator),
      agg_(agg),
      field_(-1) {}

void SumAggregator::AddFields(StructBuilder& fields) {
  field_ = fields.Add(agg_.Type(), agg_.Nullable());
}

void SumAggregator::AddInitialEntry(std::vector<SQLValue>& values) {
  values.push_back(expr_translator_.Compute(agg_.Child()));
}

void SumAggregator::Update(std::vector<SQLValue>& current_values,
                           Struct& entry) {
  const auto& current_value = current_values[field_];
  auto next = expr_translator_.Compute(agg_.Child());

  If(program_, !next.IsNull(), [&] {
    // checked that it's not null so this is safe
    auto not_null_next = next.GetNotNullable();

    // if sum is null then next else current_value + next
    If(
        program_, current_value.IsNull(),
        [&]() { entry.Update(field_, not_null_next); },
        [&]() {
          switch (current_value.Type()) {
            case catalog::SqlType::SMALLINT: {
              auto& v1 = static_cast<Int16&>(current_value.Get());
              auto& v2 = static_cast<Int16&>(next.Get());
              entry.Update(field_, SQLValue(v1 + v2, Bool(program_, false)));
              break;
            }

            case catalog::SqlType::INT: {
              auto& v1 = static_cast<Int32&>(current_value.Get());
              auto& v2 = static_cast<Int32&>(next.Get());
              entry.Update(field_, SQLValue(v1 + v2, Bool(program_, false)));
              break;
            }

            case catalog::SqlType::BIGINT: {
              auto& v1 = static_cast<Int64&>(current_value.Get());
              auto& v2 = static_cast<Int64&>(next.Get());
              entry.Update(field_, SQLValue(v1 + v2, Bool(program_, false)));
              break;
            }

            case catalog::SqlType::REAL: {
              auto& v1 = static_cast<Float64&>(current_value.Get());
              auto& v2 = static_cast<Float64&>(next.Get());
              entry.Update(field_, SQLValue(v1 + v2, Bool(program_, false)));
              break;
            }

            case catalog::SqlType::BOOLEAN:
            case catalog::SqlType::DATE:
            case catalog::SqlType::TEXT:
              throw std::runtime_error("cannot compute sum of non-numeric col");
          }
        });
  });
}

MinMaxAggregator::MinMaxAggregator(
    khir::ProgramBuilder& program,
    util::Visitor<plan::ImmutableExpressionVisitor, const plan::Expression&,
                  SQLValue>& expr_translator,
    const kush::plan::AggregateExpression& agg, bool min)
    : program_(program),
      expr_translator_(expr_translator),
      agg_(agg),
      min_(min) {}

void MinMaxAggregator::AddFields(StructBuilder& fields) {
  field_ = fields.Add(agg_.Type(), agg_.Nullable());
}

void MinMaxAggregator::AddInitialEntry(std::vector<SQLValue>& values) {
  values.push_back(expr_translator_.Compute(agg_.Child()));
}

void MinMaxAggregator::Update(std::vector<SQLValue>& current_values,
                              Struct& entry) {
  const auto& current_value = current_values[field_];
  auto next = expr_translator_.Compute(agg_.Child());
  proxy::If(program_, !next.IsNull(), [&] {
    // checked that it's not null so this is safe
    auto not_null_next = next.GetNotNullable();
    proxy::If(
        program_, current_value.IsNull(),
        [&]() { entry.Update(field_, not_null_next); },
        [&]() {
          switch (current_value.Type()) {
            case catalog::SqlType::SMALLINT: {
              auto& v1 = static_cast<proxy::Int16&>(not_null_next.Get());
              auto& v2 = static_cast<proxy::Int16&>(current_value.Get());
              proxy::If(program_, min_ ? v1 < v2 : v2 < v1,
                        [&]() { entry.Update(field_, not_null_next); });
              break;
            }

            case catalog::SqlType::INT: {
              auto& v1 = static_cast<proxy::Int32&>(next.Get());
              auto& v2 = static_cast<proxy::Int32&>(current_value.Get());
              proxy::If(program_, min_ ? v1 < v2 : v2 < v1,
                        [&]() { entry.Update(field_, not_null_next); });
              break;
            }

            case catalog::SqlType::DATE:
            case catalog::SqlType::BIGINT: {
              auto& v1 = static_cast<proxy::Int64&>(next.Get());
              auto& v2 = static_cast<proxy::Int64&>(current_value.Get());
              proxy::If(program_, min_ ? v1 < v2 : v2 < v1,
                        [&]() { entry.Update(field_, not_null_next); });
              break;
            }

            case catalog::SqlType::REAL: {
              auto& v1 = static_cast<proxy::Float64&>(next.Get());
              auto& v2 = static_cast<proxy::Float64&>(current_value.Get());
              proxy::If(program_, min_ ? v1 < v2 : v2 < v1,
                        [&]() { entry.Update(field_, not_null_next); });
              break;
            }

            case catalog::SqlType::TEXT: {
              auto& v1 = static_cast<proxy::String&>(next.Get());
              auto& v2 = static_cast<proxy::String&>(current_value.Get());
              proxy::If(program_, min_ ? v1 < v2 : v2 < v1,
                        [&]() { entry.Update(field_, not_null_next); });
              break;
            }

            case catalog::SqlType::BOOLEAN:
              throw std::runtime_error("cannot compute min of non-numeric col");
          }
        });
  });
}

}  // namespace kush::compile::proxy