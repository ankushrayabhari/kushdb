#include "parse/transform/transform_function_call_expression.h"

#include <memory>
#include <stdexcept>
#include <vector>

#include "parse/expression/aggregate_expression.h"
#include "parse/expression/expression.h"
#include "parse/transform/transform_expression.h"
#include "third_party/duckdb_libpgquery/parser.h"

namespace kush::parse {

std::unique_ptr<Expression> TransformFunctionCallExpression(
    duckdb_libpgquery::PGFuncCall &func) {
  auto name_list = func.funcname;
  if (name_list->length == 2) {
    throw std::runtime_error("Unsupported schema specifier in function call.");
  }

  if (func.over != nullptr) {
    throw std::runtime_error("Window functions not supported.");
  }

  if (func.agg_filter != nullptr) {
    throw std::runtime_error("Agg filters not supported.");
  }

  if (func.agg_distinct) {
    throw std::runtime_error("DISTINCT not supported.");
  }

  if (func.agg_order != nullptr) {
    throw std::runtime_error("Aggregate order not supported.");
  }

  if (func.agg_within_group) {
    throw std::runtime_error("Aggregate within group not supported.");
  }

  std::string function_name = reinterpret_cast<duckdb_libpgquery::PGValue *>(
                                  name_list->head->data.ptr_value)
                                  ->val.str;

  std::vector<std::unique_ptr<Expression>> children;
  if (func.args != nullptr) {
    for (auto node = func.args->head; node != nullptr; node = node->next) {
      auto child_expr = (duckdb_libpgquery::PGNode *)node->data.ptr_value;
      children.push_back(TransformExpression(*child_expr));
    }
  }

  if (function_name == "min") {
    if (children.size() != 1) {
      throw std::runtime_error("Expected 1 child for MIN but got: " +
                               std::to_string(children.size()));
    }

    return std::make_unique<AggregateExpression>(AggregateType::MIN,
                                                 std::move(children[0]));
  }

  if (function_name == "max") {
    if (children.size() != 1) {
      throw std::runtime_error("Expected 1 child for MAX but got: " +
                               std::to_string(children.size()));
    }

    return std::make_unique<AggregateExpression>(AggregateType::MAX,
                                                 std::move(children[0]));
  }

  if (function_name == "avg") {
    if (children.size() != 1) {
      throw std::runtime_error("Expected 1 child for AVG but got: " +
                               std::to_string(children.size()));
    }

    return std::make_unique<AggregateExpression>(AggregateType::AVG,
                                                 std::move(children[0]));
  }

  if (function_name == "sum") {
    if (children.size() != 1) {
      throw std::runtime_error("Expected 1 child for SUM but got: " +
                               std::to_string(children.size()));
    }

    return std::make_unique<AggregateExpression>(AggregateType::SUM,
                                                 std::move(children[0]));
  }

  if (function_name == "count") {
    if (children.empty()) {
      return std::make_unique<AggregateExpression>(AggregateType::COUNT,
                                                   nullptr);
    }

    if (children.size() == 1) {
      return std::make_unique<AggregateExpression>(AggregateType::COUNT,
                                                   std::move(children[0]));
    }

    throw std::runtime_error("Expected 0-1 child for COUNT but got: " +
                             std::to_string(children.size()));
  }

  throw std::runtime_error("Unsupported function: " + function_name);
}

}  // namespace kush::parse