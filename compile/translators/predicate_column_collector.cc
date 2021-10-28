#include "compile/translators/predicate_column_collector.h"

#include <utility>
#include <vector>

#include "absl/container/flat_hash_set.h"

#include "plan/expression/aggregate_expression.h"
#include "plan/expression/arithmetic_expression.h"
#include "plan/expression/case_expression.h"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/conversion_expression.h"
#include "plan/expression/expression.h"
#include "plan/expression/expression_visitor.h"
#include "plan/expression/extract_expression.h"
#include "plan/expression/literal_expression.h"
#include "plan/expression/virtual_column_ref_expression.h"

namespace kush::compile {

std::vector<std::reference_wrapper<const plan::ColumnRefExpression>>
PredicateColumnCollector::PredicateColumns() {
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

void PredicateColumnCollector::VisitChildren(const plan::Expression& expr) {
  for (auto& child : expr.Children()) {
    child.get().Accept(*this);
  }
}

void PredicateColumnCollector::Visit(const plan::ColumnRefExpression& col_ref) {
  predicate_columns_.push_back(col_ref);
}

void PredicateColumnCollector::Visit(
    const plan::BinaryArithmeticExpression& arith) {
  VisitChildren(arith);
}

void PredicateColumnCollector::Visit(const plan::CaseExpression& case_expr) {
  VisitChildren(case_expr);
}

void PredicateColumnCollector::Visit(
    const plan::IntToFloatConversionExpression& conv) {
  VisitChildren(conv);
}

void PredicateColumnCollector::Visit(const plan::ExtractExpression& conv) {
  VisitChildren(conv);
}

void PredicateColumnCollector::Visit(
    const plan::UnaryArithmeticExpression& conv) {
  VisitChildren(conv);
}

void PredicateColumnCollector::Visit(const plan::LiteralExpression& literal) {}

void PredicateColumnCollector::Visit(const plan::AggregateExpression& agg) {
  throw std::runtime_error("Aggregates cannot appear in join predicates.");
}

void PredicateColumnCollector::Visit(
    const plan::VirtualColumnRefExpression& virtual_col_ref) {
  throw std::runtime_error(
      "Virtual column references cannot appear in join predicates.");
}

}  // namespace kush::compile