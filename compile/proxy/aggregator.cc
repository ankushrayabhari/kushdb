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

void SumAggregator::Initialize(Struct& entry) {
  entry.Update(field_, expr_translator_.Compute(agg_.Child()));
}

void SumAggregator::Update(Struct& entry) {
  auto current_value = entry.Get(field_);
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

SQLValue SumAggregator::Get(Struct& entry) { return entry.Get(field_); }

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

void MinMaxAggregator::Initialize(Struct& entry) {
  entry.Update(field_, expr_translator_.Compute(agg_.Child()));
}

void MinMaxAggregator::Update(Struct& entry) {
  auto current_value = entry.Get(field_);
  auto next = expr_translator_.Compute(agg_.Child());
  If(program_, !next.IsNull(), [&] {
    // checked that it's not null so this is safe
    auto not_null_next = next.GetNotNullable();
    If(
        program_, current_value.IsNull(),
        [&]() { entry.Update(field_, not_null_next); },
        [&]() {
          switch (current_value.Type()) {
            case catalog::SqlType::SMALLINT: {
              auto& v1 = static_cast<Int16&>(not_null_next.Get());
              auto& v2 = static_cast<Int16&>(current_value.Get());
              If(program_, min_ ? v1 < v2 : v2 < v1,
                 [&]() { entry.Update(field_, not_null_next); });
              break;
            }

            case catalog::SqlType::DATE: {
              auto& v1 = static_cast<Date&>(next.Get());
              auto& v2 = static_cast<Date&>(current_value.Get());
              If(program_, min_ ? v1 < v2 : v2 < v1,
                 [&]() { entry.Update(field_, not_null_next); });
              break;
            }

            case catalog::SqlType::INT: {
              auto& v1 = static_cast<Int32&>(next.Get());
              auto& v2 = static_cast<Int32&>(current_value.Get());
              If(program_, min_ ? v1 < v2 : v2 < v1,
                 [&]() { entry.Update(field_, not_null_next); });
              break;
            }

            case catalog::SqlType::BIGINT: {
              auto& v1 = static_cast<Int64&>(next.Get());
              auto& v2 = static_cast<Int64&>(current_value.Get());
              If(program_, min_ ? v1 < v2 : v2 < v1,
                 [&]() { entry.Update(field_, not_null_next); });
              break;
            }

            case catalog::SqlType::REAL: {
              auto& v1 = static_cast<Float64&>(next.Get());
              auto& v2 = static_cast<Float64&>(current_value.Get());
              If(program_, min_ ? v1 < v2 : v2 < v1,
                 [&]() { entry.Update(field_, not_null_next); });
              break;
            }

            case catalog::SqlType::TEXT: {
              auto& v1 = static_cast<String&>(next.Get());
              auto& v2 = static_cast<String&>(current_value.Get());
              If(program_, min_ ? v1 < v2 : v2 < v1,
                 [&]() { entry.Update(field_, not_null_next); });
              break;
            }

            case catalog::SqlType::BOOLEAN:
              throw std::runtime_error("cannot compute min of non-numeric col");
          }
        });
  });
}

SQLValue MinMaxAggregator::Get(Struct& entry) { return entry.Get(field_); }

AverageAggregator::AverageAggregator(
    khir::ProgramBuilder& program,
    util::Visitor<plan::ImmutableExpressionVisitor, const plan::Expression&,
                  SQLValue>& expr_translator,
    const kush::plan::AggregateExpression& agg)
    : program_(program), expr_translator_(expr_translator), agg_(agg) {}

void AverageAggregator::AddFields(StructBuilder& fields) {
  // value field
  value_field_ = fields.Add(catalog::SqlType::REAL, agg_.Nullable());
  // count field
  count_field_ = fields.Add(catalog::SqlType::REAL, false);
}

void AverageAggregator::Initialize(Struct& entry) {
  auto value = expr_translator_.Compute(agg_.Child());

  entry.Update(value_field_, proxy::NullableTernary<Float64>(
                                 program_, value.IsNull(),
                                 [&]() {
                                   return SQLValue(Float64(program_, 0),
                                                   Bool(program_, true));
                                 },
                                 [&]() {
                                   // checked that it's not null so this is safe
                                   return SQLValue(ToFloat(value.Get()),
                                                   Bool(program_, false));
                                 }));

  entry.Update(count_field_,
               SQLValue(Float64(program_, 1), Bool(program_, false)));
}

Float64 AverageAggregator::ToFloat(IRValue& v) {
  if (auto i = dynamic_cast<Float64*>(&v)) {
    return Float64(program_, v.Get());
  }

  if (auto i = dynamic_cast<Int8*>(&v)) {
    return Float64(program_, *i);
  }

  if (auto i = dynamic_cast<Int16*>(&v)) {
    return Float64(program_, *i);
  }

  if (auto i = dynamic_cast<Int32*>(&v)) {
    return Float64(program_, *i);
  }

  if (auto i = dynamic_cast<Int64*>(&v)) {
    return Float64(program_, *i);
  }

  throw std::runtime_error("Can't convert to float.");
}

void AverageAggregator::Update(Struct& entry) {
  auto next = expr_translator_.Compute(agg_.Child());
  If(program_, !next.IsNull(), [&] {
    // checked that it's not null so this is safe
    auto next_value = ToFloat(next.Get());

    auto record_count_field = entry.Get(count_field_);
    const auto& record_count = static_cast<Float64&>(record_count_field.Get());
    auto current_value = entry.Get(value_field_);
    If(
        program_, current_value.IsNull(),
        [&]() {
          // since current_value is null, record counter is 1
          // record counter stays at 1 and field gets updated
          entry.Update(value_field_,
                       SQLValue(next_value, Bool(program_, false)));
        },
        [&]() {
          // checked that it's not null so this is safe
          const auto& cma = static_cast<Float64&>(current_value.Get());

          auto next_record_count = record_count + 1;
          auto next_cma = cma + (next_value - cma) / next_record_count;

          entry.Update(count_field_,
                       SQLValue(next_record_count, Bool(program_, false)));
          entry.Update(value_field_, SQLValue(next_cma, Bool(program_, false)));
        });
  });
}

SQLValue AverageAggregator::Get(Struct& entry) {
  return entry.Get(value_field_);
}

CountAggregator::CountAggregator(
    khir::ProgramBuilder& program,
    util::Visitor<plan::ImmutableExpressionVisitor, const plan::Expression&,
                  SQLValue>& expr_translator,
    const kush::plan::AggregateExpression& agg)
    : program_(program), expr_translator_(expr_translator), agg_(agg) {}

void CountAggregator::AddFields(StructBuilder& fields) {
  field_ = fields.Add(catalog::SqlType::BIGINT, false);
}

void CountAggregator::Initialize(Struct& entry) {
  entry.Update(field_, SQLValue(Int64(program_, 1), Bool(program_, false)));
}

void CountAggregator::Update(Struct& entry) {
  auto next = expr_translator_.Compute(agg_.Child());
  If(program_, !next.IsNull(), [&] {
    auto record_count_field = entry.Get(field_);
    auto record_count = static_cast<Int64&>(record_count_field.Get());
    entry.Update(field_, SQLValue(record_count + 1, Bool(program_, false)));
  });
}

SQLValue CountAggregator::Get(Struct& entry) { return entry.Get(field_); }

}  // namespace kush::compile::proxy