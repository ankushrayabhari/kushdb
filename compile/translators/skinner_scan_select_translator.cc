#include "compile/translators/skinner_scan_select_translator.h"

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"

#include "compile/proxy/column_data.h"
#include "compile/proxy/control_flow/if.h"
#include "compile/proxy/control_flow/loop.h"
#include "compile/proxy/skinner_scan_select_executor.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "compile/translators/predicate_column_collector.h"
#include "khir/program_builder.h"
#include "plan/operator/skinner_scan_select_operator.h"

namespace kush::compile {

class PredicateFunction {
 public:
  PredicateFunction(khir::ProgramBuilder& program,
                    std::function<void(PredicateFunction& p)> body)
      : program_(program) {
    auto current_block = program_.CurrentBlock();
    func_ = program_.CreatePublicFunction(
        program_.I1Type(), {},
        "permute_predicate_" + std::to_string(predicate_++));
    body(*this);
    program_.SetCurrentBlock(current_block);
  }

  void Return(const proxy::Bool& b) { program_.Return(b.Get()); }

  khir::FunctionRef Get() { return func_.value(); }

 private:
  khir::ProgramBuilder& program_;
  std::optional<khir::FunctionRef> func_;
  static int predicate_;
};

int PredicateFunction::predicate_ = 0;

class BaseFunction {
 public:
  BaseFunction(khir::ProgramBuilder& program, std::function<void()> body)
      : program_(program) {
    auto current_block = program_.CurrentBlock();
    func_ = program_.CreatePublicFunction(program_.VoidType(), {},
                                          "base_" + std::to_string(funcid_++));

    body();
    program_.Return();
    program_.SetCurrentBlock(current_block);
  }

  khir::FunctionRef Get() { return func_.value(); }

 private:
  khir::ProgramBuilder& program_;
  std::optional<khir::FunctionRef> func_;
  static int funcid_;
};
int BaseFunction::funcid_ = 0;

SkinnerScanSelectTranslator::SkinnerScanSelectTranslator(
    const plan::SkinnerScanSelectOperator& scan_select,
    khir::ProgramBuilder& program)
    : OperatorTranslator(scan_select, {}),
      scan_select_(scan_select),
      program_(program),
      expr_translator_(program, *this) {}

std::unique_ptr<proxy::DiskMaterializedBuffer>
SkinnerScanSelectTranslator::GenerateBuffer() {
  const auto& table = scan_select_.Relation();
  const auto& cols = scan_select_.ScanSchema().Columns();
  auto num_cols = cols.size();

  std::vector<std::unique_ptr<proxy::Iterable>> column_data;
  std::vector<std::unique_ptr<proxy::Iterable>> null_data;
  column_data.reserve(num_cols);
  null_data.reserve(num_cols);
  for (const auto& column : cols) {
    using catalog::SqlType;
    auto type = column.Expr().Type();
    auto path = table[column.Name()].Path();
    switch (type) {
      case SqlType::SMALLINT:
        column_data.push_back(
            std::make_unique<proxy::ColumnData<SqlType::SMALLINT>>(program_,
                                                                   path));
        break;
      case SqlType::INT:
        column_data.push_back(
            std::make_unique<proxy::ColumnData<SqlType::INT>>(program_, path));
        break;
      case SqlType::BIGINT:
        column_data.push_back(
            std::make_unique<proxy::ColumnData<SqlType::BIGINT>>(program_,
                                                                 path));
        break;
      case SqlType::REAL:
        column_data.push_back(
            std::make_unique<proxy::ColumnData<SqlType::REAL>>(program_, path));
        break;
      case SqlType::DATE:
        column_data.push_back(
            std::make_unique<proxy::ColumnData<SqlType::DATE>>(program_, path));
        break;
      case SqlType::TEXT:
        column_data.push_back(
            std::make_unique<proxy::ColumnData<SqlType::TEXT>>(program_, path));
        break;
      case SqlType::BOOLEAN:
        column_data.push_back(
            std::make_unique<proxy::ColumnData<SqlType::BOOLEAN>>(program_,
                                                                  path));
        break;
    }

    if (table[column.Name()].Nullable()) {
      null_data.push_back(std::make_unique<proxy::ColumnData<SqlType::BOOLEAN>>(
          program_, table[column.Name()].NullPath()));
    } else {
      null_data.push_back(nullptr);
    }
  }

  return std::make_unique<proxy::DiskMaterializedBuffer>(
      program_, std::move(column_data), std::move(null_data));
}

void SkinnerScanSelectTranslator::Produce() {
  const auto& scan_schema_columns = scan_select_.ScanSchema().Columns();
  auto conditions = scan_select_.Filters();

  auto materialized_buffer = GenerateBuffer();
  materialized_buffer->Init();

  // 1. Setup predicate evaluation
  // Collect predicate columns.
  absl::flat_hash_set<int> predicate_columns;
  for (int i = 0; i < conditions.size(); i++) {
    const auto& condition = conditions[i].get();

    ScanSelectPredicateColumnCollector collector;
    condition.Accept(collector);
    auto cond = collector.PredicateColumns();

    for (const auto& col_ref : cond) {
      auto key = col_ref.get().GetColumnIdx();
      if (!predicate_columns.contains(key)) {
        predicate_columns.insert(key);
      }
    }
  }

  // Generate predicate struct.
  auto predicate_struct = std::make_unique<proxy::StructBuilder>(program_);
  absl::flat_hash_map<int, int> col_to_predicate_struct_field;
  int pred_struct_size = 0;
  for (auto column_idx : predicate_columns) {
    col_to_predicate_struct_field[column_idx] = pred_struct_size++;
    const auto& column_schema = scan_schema_columns[column_idx];
    predicate_struct->Add(column_schema.Expr().Type(),
                          column_schema.Expr().Nullable());
  }
  predicate_struct->Build();

  proxy::Struct global_predicate_struct(
      program_, *predicate_struct,
      program_.Global(
          false, true, predicate_struct->Type(),
          program_.ConstantStruct(predicate_struct->Type(),
                                  predicate_struct->DefaultValues())));

  this->virtual_values_.ResetValues();
  for (int i = 0; i < scan_schema_columns.size(); i++) {
    switch (scan_schema_columns[i].Expr().Type()) {
      case catalog::SqlType::SMALLINT:
        this->virtual_values_.AddVariable(proxy::SQLValue(
            proxy::Int16(program_, 0), proxy::Bool(program_, false)));
        break;
      case catalog::SqlType::INT:
        this->virtual_values_.AddVariable(proxy::SQLValue(
            proxy::Int32(program_, 0), proxy::Bool(program_, false)));
        break;
      case catalog::SqlType::DATE:
        this->virtual_values_.AddVariable(
            proxy::SQLValue(proxy::Date(program_, absl::CivilDay(2000, 1, 1)),
                            proxy::Bool(program_, false)));
        break;
      case catalog::SqlType::BIGINT:
        this->virtual_values_.AddVariable(proxy::SQLValue(
            proxy::Int64(program_, 0), proxy::Bool(program_, false)));
        break;
      case catalog::SqlType::BOOLEAN:
        this->virtual_values_.AddVariable(proxy::SQLValue(
            proxy::Bool(program_, false), proxy::Bool(program_, false)));
        break;
      case catalog::SqlType::REAL:
        this->virtual_values_.AddVariable(proxy::SQLValue(
            proxy::Float64(program_, 0), proxy::Bool(program_, false)));
        break;
      case catalog::SqlType::TEXT:
        this->virtual_values_.AddVariable(proxy::SQLValue(
            proxy::String::Global(program_, ""), proxy::Bool(program_, false)));
        break;
    }
  }

  // Generate function that will evaluate each predicate.
  std::vector<PredicateFunction> predicate_fns;
  for (int i = 0; i < conditions.size(); i++) {
    predicate_fns.emplace_back(program_, [&](auto& func) {
      // Unpack the predicate struct.
      auto column_values = global_predicate_struct.Unpack();

      for (const auto& [col_idx, field] : col_to_predicate_struct_field) {
        // Set the ColumnIdx value of the ChildIdx operator to be the
        // unpacked value.
        this->virtual_values_.SetValue(col_idx, column_values[field]);
      }

      auto value = expr_translator_.Compute(conditions[i].get());

      proxy::If(program_, value.IsNull(),
                [&]() { func.Return(proxy::Bool(program_, false)); });

      func.Return(static_cast<proxy::Bool&>(value.Get()));
    });
  }

  // Generate main function that will loop over index buckets
  // Loop over all indexed predicates
  BaseFunction main_func(program_, [&]() {
    auto cardinality = materialized_buffer->Size();
    proxy::Loop loop(
        program_,
        [&](auto& loop) { loop.AddLoopVariable(proxy::Int32(program_, 0)); },
        [&](auto& loop) {
          auto i = loop.template GetLoopVariable<proxy::Int32>(0);
          return i < cardinality;
        },
        [&](auto& loop) {
          auto i = loop.template GetLoopVariable<proxy::Int32>(0);

          // Get ith value
          auto values = (*materialized_buffer)[i];

          // Store each of this table's predicate column
          // values into the global_predicate_struct.
          for (auto [col_idx, field] : col_to_predicate_struct_field) {
            global_predicate_struct.Update(field, values[col_idx]);
          }

          // Check all predicates
          for (auto& predicate : predicate_fns) {
            proxy::Bool value(program_, program_.Call(predicate.Get(), {}));
            proxy::If(program_, !value, [&]() { loop.Continue(i + 1); });
          }

          this->virtual_values_.SetValues(values);
          for (const auto& column : scan_select_.Schema().Columns()) {
            this->values_.AddVariable(expr_translator_.Compute(column.Expr()));
          }

          if (auto parent = this->Parent()) {
            parent->get().Consume(*this);
          }

          return loop.Continue(i + 1);
        });
  });

  program_.Call(main_func.Get(), {});
  materialized_buffer->Reset();
}

void SkinnerScanSelectTranslator::Consume(OperatorTranslator& src) {
  throw std::runtime_error("Scan cannot consume tuples - leaf operator");
}

}  // namespace kush::compile