#include "plan/operator/operator_schema.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "magic_enum.hpp"

#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "nlohmann/json.hpp"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/expression.h"
#include "plan/expression/virtual_column_ref_expression.h"

namespace kush::plan {

OperatorSchema::Column::Column(std::string_view name,
                               std::unique_ptr<Expression> expr)
    : name_(name), expr_(std::move(expr)) {}

std::string_view OperatorSchema::Column::Name() const { return name_; }

const Expression& OperatorSchema::Column::Expr() const { return *expr_; }

Expression& OperatorSchema::Column::MutableExpr() { return *expr_; }

std::unique_ptr<Expression> OperatorSchema::Column::DestroyExpr() {
  return std::move(expr_);
}

void OperatorSchema::Column::SetExpr(std::unique_ptr<Expression> e) {
  expr_ = std::move(e);
}

void OperatorSchema::AddDerivedColumn(std::string_view name,
                                      std::unique_ptr<Expression> expr) {
  column_name_to_idx_[name] = columns_.size();
  columns_.emplace_back(name, std::move(expr));
}

void OperatorSchema::AddGeneratedColumns(const kush::catalog::Table& table) {
  for (auto catalog_column : table.Columns()) {
    auto name = catalog_column.get().Name();
    auto type = catalog_column.get().GetType();
    int idx = columns_.size();
    column_name_to_idx_[name] = idx;
    columns_.emplace_back(name,
                          std::make_unique<ColumnRefExpression>(
                              type, catalog_column.get().Nullable(), 0, idx));
  }
}

void OperatorSchema::AddGeneratedColumns(
    const kush::catalog::Table& table,
    const std::vector<std::string>& columns) {
  for (const auto& name : columns) {
    const auto& catalog_column = table[name];
    auto type = catalog_column.GetType();
    int idx = columns_.size();
    column_name_to_idx_[name] = idx;
    columns_.emplace_back(name, std::make_unique<ColumnRefExpression>(
                                    type, catalog_column.Nullable(), 0, idx));
  }
}

void OperatorSchema::AddPassthroughColumns(const OperatorSchemaProvider& op,
                                           int child_idx) {
  const OperatorSchema& schema = op.Schema();
  for (int i = 0; i < schema.Columns().size(); i++) {
    const auto& col = schema.Columns()[i];
    auto type = col.Expr().Type();
    auto nullable = col.Expr().Nullable();
    AddDerivedColumn(col.Name(), std::make_unique<ColumnRefExpression>(
                                     type, nullable, child_idx, i));
  }
}

void OperatorSchema::AddPassthroughColumns(
    const OperatorSchemaProvider& op, const std::vector<std::string>& columns,
    int child_idx) {
  const OperatorSchema& schema = op.Schema();
  for (const auto& name : columns) {
    auto idx = schema.GetColumnIndex(name);
    const auto& col = schema.Columns()[idx];
    auto type = col.Expr().Type();
    auto nullable = col.Expr().Nullable();
    AddDerivedColumn(name, std::make_unique<ColumnRefExpression>(
                               type, nullable, child_idx, idx));
  }
}

void OperatorSchema::AddVirtualPassthroughColumns(
    const OperatorSchema& schema) {
  for (const auto& col : schema.Columns()) {
    auto type = col.Expr().Type();
    auto nullable = col.Expr().Nullable();
    auto name = col.Name();
    auto idx = schema.GetColumnIndex(name);
    AddDerivedColumn(name, std::make_unique<VirtualColumnRefExpression>(
                               type, nullable, idx));
  }
}

void OperatorSchema::AddVirtualPassthroughColumns(
    const OperatorSchema& schema, const std::vector<std::string>& columns) {
  for (const auto& name : columns) {
    auto idx = schema.GetColumnIndex(name);
    const auto& col = schema.Columns()[idx];
    auto type = col.Expr().Type();
    auto nullable = col.Expr().Nullable();
    AddDerivedColumn(name, std::make_unique<VirtualColumnRefExpression>(
                               type, nullable, idx));
  }
}

void OperatorSchema::AddPassthroughColumn(const OperatorSchemaProvider& op,
                                          std::string_view base_name,
                                          std::string_view derived_name,
                                          int child_idx) {
  const OperatorSchema& schema = op.Schema();
  auto idx = schema.GetColumnIndex(base_name);
  const auto& col = schema.Columns()[idx];
  auto type = col.Expr().Type();
  auto nullable = col.Expr().Nullable();
  AddDerivedColumn(derived_name, std::make_unique<ColumnRefExpression>(
                                     type, nullable, child_idx, idx));
}

const std::vector<OperatorSchema::Column>& OperatorSchema::Columns() const {
  return columns_;
}

std::vector<OperatorSchema::Column>& OperatorSchema::MutableColumns() {
  return columns_;
}

int OperatorSchema::GetColumnIndex(std::string_view name) const {
  return column_name_to_idx_.at(name);
}

void OperatorSchema::RemoveColumn(int idx) {
  column_name_to_idx_.erase(columns_[idx].Name());
  columns_.erase(columns_.begin() + idx);
}

nlohmann::json OperatorSchema::ToJson() const {
  nlohmann::json j;
  for (const Column& c : columns_) {
    nlohmann::json c_json;
    c_json["name"] = c.Name();
    c_json["value"] = c.Expr().ToJson();
    j["columns"].push_back(c_json);
  }
  return j;
}

}  // namespace kush::plan
