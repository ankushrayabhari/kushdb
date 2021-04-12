#include "compile/translators/skinner_join_translator.h"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "compile/ir_registry.h"
#include "compile/proxy/function.h"
#include "compile/proxy/loop.h"
#include "compile/proxy/printer.h"
#include "compile/proxy/struct.h"
#include "compile/proxy/value.h"
#include "compile/proxy/vector.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/operator_translator.h"
#include "compile/translators/scan_translator.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/expression/binary_arithmetic_expression.h"
#include "plan/expression/case_expression.h"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/conversion_expression.h"
#include "plan/expression/expression_visitor.h"
#include "plan/expression/literal_expression.h"
#include "plan/expression/virtual_column_ref_expression.h"
#include "util/vector_util.h"

namespace kush::compile {

class PredicateColumnCollector : public plan::ImmutableExpressionVisitor {
 public:
  PredicateColumnCollector() = default;
  virtual ~PredicateColumnCollector() = default;

  std::vector<std::reference_wrapper<const plan::ColumnRefExpression>>
  PredicateColumns() {
    // dedupe, should actually be doing this via hashing
    absl::flat_hash_set<std::pair<int, int>> exists;

    std::vector<std::reference_wrapper<const plan::ColumnRefExpression>> output;
    for (auto& col_ref : predicate_columns_) {
      std::pair<int, int> key = {col_ref.get().GetChildIdx(),
                                 col_ref.get().GetColumnIdx()};
      if (exists.contains(key)) {
        continue;
      }

      output.push_back(col_ref);
      exists.insert(key);
    }
    return output;
  }

  void Visit(const plan::ColumnRefExpression& col_ref) override {
    predicate_columns_.push_back(col_ref);
  }

  void VisitChildren(const plan::Expression& expr) {
    for (auto& child : expr.Children()) {
      child.get().Accept(*this);
    }
  }

  void Visit(const plan::BinaryArithmeticExpression& arith) override {
    VisitChildren(arith);
  }

  void Visit(const plan::CaseExpression& case_expr) override {
    VisitChildren(case_expr);
  }

  void Visit(const plan::IntToFloatConversionExpression& conv) override {
    VisitChildren(conv);
  }

  void Visit(const plan::LiteralExpression& literal) override {}

  void Visit(const plan::AggregateExpression& agg) override {
    throw std::runtime_error("Aggregates cannot appear in join predicates.");
  }

  void Visit(const plan::VirtualColumnRefExpression& virtual_col_ref) override {
    throw std::runtime_error(
        "Virtual column references cannot appear in join predicates.");
  }

 private:
  std::vector<std::reference_wrapper<const plan::ColumnRefExpression>>
      predicate_columns_;
};

template <typename T>
SkinnerJoinTranslator<T>::SkinnerJoinTranslator(
    const plan::SkinnerJoinOperator& join, ProgramBuilder<T>& program,
    std::vector<std::unique_ptr<OperatorTranslator<T>>> children)
    : OperatorTranslator<T>(std::move(children)),
      join_(join),
      program_(program),
      expr_translator_(program_, *this) {}

template <typename T>
void SkinnerJoinTranslator<T>::Produce() {
  // 1. Materialize each child.
  auto child_translators = this->Children();
  auto child_operators = this->join_.Children();
  auto conditions = join_.Conditions();

  std::vector<std::unique_ptr<proxy::StructBuilder<T>>> structs;
  for (int i = 0; i < child_translators.size(); i++) {
    auto& child_translator = child_translators[i].get();
    auto& child_operator = child_operators[i].get();

    if (auto scan = dynamic_cast<ScanTranslator<T>*>(&child_translator)) {
      // Already materialized so do nothing.
    } else {
      // Create struct for materialization
      structs.push_back(std::make_unique<proxy::StructBuilder<T>>(program_));
      const auto& child_schema = child_operator.Schema().Columns();
      for (const auto& col : child_schema) {
        structs.back()->Add(col.Expr().Type());
      }
      structs.back()->Build();

      // Init vector of structs
      buffers_.push_back(
          std::make_unique<proxy::Vector<T>>(program_, *structs.back(), true));

      // Fill buffer
      child_translator.Produce();
    }
  }

  // 2. Setup join evaluation
  // Setup struct of predicate columns
  PredicateColumnCollector collector;
  for (const auto& condition : conditions) {
    condition.get().Accept(collector);
  }

  auto predicate_columns = collector.PredicateColumns();
  predicate_struct_ = std::make_unique<proxy::StructBuilder<T>>(program_);

  for (auto& predicate_column : predicate_columns) {
    const auto& child = child_operators[predicate_column.get().GetChildIdx()];
    const auto& col =
        child.get().Schema().Columns()[predicate_column.get().GetColumnIdx()];
    auto type = col.Expr().Type();
    predicate_struct_->Add(type);
  }
  predicate_struct_->Build();

  auto& global_predicate_struct = program_.GlobalStruct(
      false, predicate_struct_->Type(), predicate_struct_->DefaultValues());

  // Setup table handlers
  auto& handler_type = program_.FunctionType(program_.VoidType(), {});
  auto& handler_pointer_type = program_.PointerType(handler_type);
  auto& handler_pointer_array_type =
      program_.ArrayType(handler_pointer_type, child_translators.size());

  std::vector<std::reference_wrapper<typename ProgramBuilder<T>::Value>>
      initial_values;
  initial_values.reserve(child_translators.size());
  for (int i = 0; i < child_translators.size(); i++) {
    initial_values.push_back(program_.NullPtr(handler_pointer_type));
  }
  auto& handler_pointer_array =
      program_.GlobalArray(false, handler_pointer_array_type, initial_values);

  std::vector<proxy::VoidFunction<T>> table_functions;
  int current_buffer = 0;
  for (int i = 0; i < child_translators.size(); i++) {
    auto& child_translator = child_translators[i].get();
    auto& child_operator = child_operators[i].get();

    proxy::VoidFunction<T>(program_, [&]() {
      auto& handler_ptr = program_.GetElementPtr(
          handler_pointer_array_type, handler_pointer_array,
          {program_.ConstI32(0), program_.ConstI32(i)});
      auto& handler = program_.Load(handler_ptr);

      if (auto scan = dynamic_cast<ScanTranslator<T>*>(&child_translator)) {
        // Loop over tuples in column data
        program_.Return();
      } else {
        // Loop over tuples in buffer
        proxy::Vector<T>& buffer = *buffers_[current_buffer++];

        proxy::IndexLoop<T>(
            program_, [&]() { return proxy::UInt32<T>(program_, 0); },
            [&](proxy::UInt32<T>& i) { return i < buffer.Size(); },
            [&](proxy::UInt32<T>& i) {
              // TODO: store this tables values into the
              // global_predicate_struct

              // Call next table handler
              program_.Call(handler, handler_type);

              return i + proxy::UInt32<T>(program_, 1);
            });
        program_.Return();
      }
    });
  }

  // TODO: actually invoke the predicates.

  // 3. Execute join

  // Cleanup
  for (auto& buffer : buffers_) {
    if (buffer != nullptr) {
      buffer.reset();
    }
  }
  predicate_struct_.reset();
}

template <typename T>
void SkinnerJoinTranslator<T>::Consume(OperatorTranslator<T>& src) {
  // Last element of buffers is the materialization vector for src.
  buffers_.back()->PushBack().Pack(src.SchemaValues().Values());
}

INSTANTIATE_ON_IR(SkinnerJoinTranslator);

}  // namespace kush::compile