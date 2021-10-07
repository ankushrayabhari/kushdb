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
  switch (agg_.Type()) {
    case catalog::SqlType::SMALLINT:
    case catalog::SqlType::INT:
    case catalog::SqlType::BIGINT:
    case catalog::SqlType::REAL:
      field_ = fields.Add(agg_.Type(), agg_.Nullable());
      break;

    default:
      throw std::runtime_error("Not able to aggregate this type");
  }
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
        program_, current_value.IsNull(), [&]() { entry.Update(field_, next); },
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

}  // namespace kush::compile::proxy