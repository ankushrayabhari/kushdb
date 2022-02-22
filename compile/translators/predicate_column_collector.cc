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

void PredicateColumnCollector::Visit(
    const plan::RegexpMatchingExpression& match) {
  VisitChildren(match);
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

std::vector<std::reference_wrapper<const plan::VirtualColumnRefExpression>>
ScanSelectPredicateColumnCollector::PredicateColumns() {
  // dedupe, should actually be doing this via hashing
  absl::flat_hash_set<int> exists;

  std::vector<std::reference_wrapper<const plan::VirtualColumnRefExpression>>
      output;
  for (auto& col_ref : predicate_columns_) {
    auto key = col_ref.get().GetColumnIdx();
    if (exists.contains(key)) {
      continue;
    }

    output.push_back(col_ref);
    exists.insert(key);
  }
  return output;
}

void ScanSelectPredicateColumnCollector::VisitChildren(
    const plan::Expression& expr) {
  for (auto& child : expr.Children()) {
    child.get().Accept(*this);
  }
}

void ScanSelectPredicateColumnCollector::Visit(
    const plan::ColumnRefExpression& col_ref) {
  throw std::runtime_error(
      "Column references cannot appear in scan/select predicates.");
}

void ScanSelectPredicateColumnCollector::Visit(
    const plan::BinaryArithmeticExpression& arith) {
  VisitChildren(arith);
}

void ScanSelectPredicateColumnCollector::Visit(
    const plan::CaseExpression& case_expr) {
  VisitChildren(case_expr);
}

void ScanSelectPredicateColumnCollector::Visit(
    const plan::IntToFloatConversionExpression& conv) {
  VisitChildren(conv);
}

void ScanSelectPredicateColumnCollector::Visit(
    const plan::ExtractExpression& conv) {
  VisitChildren(conv);
}

void ScanSelectPredicateColumnCollector::Visit(
    const plan::UnaryArithmeticExpression& conv) {
  VisitChildren(conv);
}

void ScanSelectPredicateColumnCollector::Visit(
    const plan::RegexpMatchingExpression& match) {
  VisitChildren(match);
}

void ScanSelectPredicateColumnCollector::Visit(
    const plan::LiteralExpression& literal) {}

void ScanSelectPredicateColumnCollector::Visit(
    const plan::AggregateExpression& agg) {
  throw std::runtime_error("Aggregates cannot appear in join predicates.");
}

void ScanSelectPredicateColumnCollector::Visit(
    const plan::VirtualColumnRefExpression& virtual_col_ref) {
  predicate_columns_.push_back(virtual_col_ref);
}

}  // namespace kush::compile