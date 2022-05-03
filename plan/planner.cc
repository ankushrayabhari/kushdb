#include "plan/planner.h"

#include <iostream>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"

#include "re2/re2.h"

#include "catalog/catalog_manager.h"
#include "parse/expression/aggregate_expression.h"
#include "parse/expression/arithmetic_expression.h"
#include "parse/expression/case_expression.h"
#include "parse/expression/column_ref_expression.h"
#include "parse/expression/expression.h"
#include "parse/expression/in_expression.h"
#include "parse/expression/literal_expression.h"
#include "parse/statement/select_statement.h"
#include "parse/table/table.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/expression/arithmetic_expression.h"
#include "plan/expression/literal_expression.h"
#include "plan/expression/virtual_column_ref_expression.h"
#include "plan/operator/aggregate_operator.h"
#include "plan/operator/operator.h"
#include "plan/operator/output_operator.h"
#include "plan/operator/scan_operator.h"
#include "plan/operator/select_operator.h"
#include "plan/operator/skinner_join_operator.h"
#include "plan/operator/skinner_scan_select_operator.h"
#include "util/vector_util.h"

namespace kush::plan {

void GetReferencedChildren(const Expression& expr,
                           absl::flat_hash_set<std::pair<int, int>>& result) {
  if (auto v = dynamic_cast<const ColumnRefExpression*>(&expr)) {
    result.emplace(v->GetChildIdx(), v->GetColumnIdx());
    return;
  }

  for (auto c : expr.Children()) {
    GetReferencedChildren(c.get(), result);
  }
}

void RewriteColumnReferences(
    Expression& expr,
    const absl::flat_hash_map<std::pair<int, int>, std::pair<int, int>>&
        col_ref_to_rewrite_idx) {
  if (auto v = dynamic_cast<ColumnRefExpression*>(&expr)) {
    std::pair<int, int> key{v->GetChildIdx(), v->GetColumnIdx()};
    if (col_ref_to_rewrite_idx.contains(key)) {
      const auto& value = col_ref_to_rewrite_idx.at(key);
      v->SetChildIdx(value.first);
      v->SetColumnIdx(value.second);
    }
    return;
  }

  for (auto c : expr.MutableChildren()) {
    RewriteColumnReferences(c.get(), col_ref_to_rewrite_idx);
  }
}

std::vector<std::unique_ptr<Operator>> Planner::Plan(const parse::Table& table,
                                                     int child_idx) {
  if (auto base_table = dynamic_cast<const parse::BaseTable*>(&table)) {
    auto table_name = base_table->Name();
    auto alias = base_table->Alias();
    const auto& db = catalog::CatalogManager::Get().Current();
    if (!db.Contains(table_name)) {
      std::string result = "Unknown table ";
      result += table_name;
      throw std::runtime_error(result);
    }

    const auto& table = db[table_name];

    if (alias.empty()) {
      alias = table.Name();
    }

    int col_idx = 0;
    for (const auto& col : table.Columns()) {
      std::pair<std::string, std::string> key;
      key.first = alias;
      key.second = col.get().Name();

      auto& v = table_col_to_info_[key];
      v.child_idx = child_idx;
      v.col_idx = col_idx++;
      v.type = col.get().GetType();
      v.nullable = col.get().Nullable();
    }

    OperatorSchema schema;
    schema.AddGeneratedColumns(table);
    std::unique_ptr<Operator> op =
        std::make_unique<ScanOperator>(std::move(schema), table);
    return util::MakeVector(std::move(op));
  }

  if (auto cross_product =
          dynamic_cast<const parse::CrossProductTable*>(&table)) {
    std::vector<std::unique_ptr<Operator>> children;

    int child_idx = 0;
    for (auto table : cross_product->Children()) {
      for (auto& x : Plan(table.get(), child_idx)) {
        children.push_back(std::move(x));
      }
      child_idx++;
    }

    return children;
  }

  throw std::runtime_error("Unknown table type.");
}

std::unique_ptr<Expression> Planner::Plan(
    const parse::AggregateExpression& expr) {
  AggregateType type;
  switch (expr.Type()) {
    case parse::AggregateType::COUNT:
      type = AggregateType::COUNT;
      break;
    case parse::AggregateType::MAX:
      type = AggregateType::MAX;
      break;
    case parse::AggregateType::MIN:
      type = AggregateType::MIN;
      break;
    case parse::AggregateType::AVG:
      type = AggregateType::AVG;
      break;
    case parse::AggregateType::SUM:
      type = AggregateType::SUM;
      break;
    default:
      break;
  }
  return std::make_unique<AggregateExpression>(type, Plan(expr.Child()));
}

std::unique_ptr<Expression> Planner::Plan(
    const parse::UnaryArithmeticExpression& expr) {
  switch (expr.Type()) {
    case parse::UnaryArithmeticExpressionType::NOT:
      return std::make_unique<UnaryArithmeticExpression>(
          UnaryArithmeticExpressionType::NOT, Plan(expr.Child()));

    case parse::UnaryArithmeticExpressionType::IS_NULL:
      return std::make_unique<UnaryArithmeticExpression>(
          UnaryArithmeticExpressionType::IS_NULL, Plan(expr.Child()));
  }
}

std::unique_ptr<Expression> Planner::Plan(
    const parse::BinaryArithmeticExpression& expr) {
  switch (expr.Type()) {
    case parse::BinaryArithmeticExpressionType::AND:
      return std::make_unique<BinaryArithmeticExpression>(
          BinaryArithmeticExpressionType::AND, Plan(expr.LeftChild()),
          Plan(expr.RightChild()));

    case parse::BinaryArithmeticExpressionType::OR:
      return std::make_unique<BinaryArithmeticExpression>(
          BinaryArithmeticExpressionType::OR, Plan(expr.LeftChild()),
          Plan(expr.RightChild()));

    case parse::BinaryArithmeticExpressionType::EQ: {
      auto left = Plan(expr.LeftChild());
      auto right = Plan(expr.RightChild());

      if (left->Type().type_id == catalog::TypeId::ENUM) {
        if (auto r =
                dynamic_cast<kush::plan::LiteralExpression*>(right.get())) {
          std::string literal;
          r->Visit(
              nullptr, nullptr, nullptr, nullptr,
              [&](auto x, bool n) { literal = x; }, nullptr, nullptr, nullptr);
          auto r_rewrite = std::make_unique<LiteralExpression>(
              left->Type().enum_id, literal);
          return std::make_unique<kush::plan::BinaryArithmeticExpression>(
              kush::plan::BinaryArithmeticExpressionType::EQ, std::move(left),
              std::move(r_rewrite));
        }
      }

      return std::make_unique<BinaryArithmeticExpression>(
          BinaryArithmeticExpressionType::EQ, std::move(left),
          std::move(right));
    }

    case parse::BinaryArithmeticExpressionType::NEQ: {
      auto left = Plan(expr.LeftChild());
      auto right = Plan(expr.RightChild());

      if (left->Type().type_id == catalog::TypeId::ENUM) {
        if (auto r =
                dynamic_cast<kush::plan::LiteralExpression*>(right.get())) {
          std::string literal;
          r->Visit(
              nullptr, nullptr, nullptr, nullptr,
              [&](auto x, bool n) { literal = x; }, nullptr, nullptr, nullptr);
          auto r_rewrite = std::make_unique<LiteralExpression>(
              left->Type().enum_id, literal);
          return std::make_unique<kush::plan::BinaryArithmeticExpression>(
              kush::plan::BinaryArithmeticExpressionType::NEQ, std::move(left),
              std::move(r_rewrite));
        }
      }

      return std::make_unique<BinaryArithmeticExpression>(
          BinaryArithmeticExpressionType::NEQ, std::move(left),
          std::move(right));
    }

    case parse::BinaryArithmeticExpressionType::LT:
      return std::make_unique<BinaryArithmeticExpression>(
          BinaryArithmeticExpressionType::LT, Plan(expr.LeftChild()),
          Plan(expr.RightChild()));

    case parse::BinaryArithmeticExpressionType::LEQ:
      return std::make_unique<BinaryArithmeticExpression>(
          BinaryArithmeticExpressionType::LEQ, Plan(expr.LeftChild()),
          Plan(expr.RightChild()));

    case parse::BinaryArithmeticExpressionType::GT:
      return std::make_unique<BinaryArithmeticExpression>(
          BinaryArithmeticExpressionType::GT, Plan(expr.LeftChild()),
          Plan(expr.RightChild()));

    case parse::BinaryArithmeticExpressionType::GEQ:
      return std::make_unique<BinaryArithmeticExpression>(
          BinaryArithmeticExpressionType::GEQ, Plan(expr.LeftChild()),
          Plan(expr.RightChild()));

    case parse::BinaryArithmeticExpressionType::LIKE: {
      const auto& right_parsed = expr.RightChild();
      if (auto literal =
              dynamic_cast<const parse::LiteralExpression*>(&right_parsed)) {
        auto match = literal->GetValue();

        if (match.size() >= 1 && match.front() == '%') {
          std::string_view match_info(match);
          match_info.remove_prefix(1);

          if (match_info.find('%') == std::string::npos) {
            return std::make_unique<BinaryArithmeticExpression>(
                BinaryArithmeticExpressionType::STARTS_WITH,
                Plan(expr.LeftChild()),
                std::make_unique<LiteralExpression>(match_info));
          }
        }

        if (match.size() >= 1 && match.back() == '%') {
          std::string_view match_info(match);
          match_info.remove_suffix(1);
          if (match_info.find('%') == std::string::npos) {
            return std::make_unique<BinaryArithmeticExpression>(
                BinaryArithmeticExpressionType::ENDS_WITH,
                Plan(expr.LeftChild()),
                std::make_unique<LiteralExpression>(match_info));
          }
        }

        if (match.size() >= 2 && match.front() == '%' && match.back() == '%') {
          std::string_view match_info(match);
          match_info.remove_prefix(1);
          match_info.remove_suffix(1);
          if (match_info.find('%') == std::string::npos) {
            return std::make_unique<BinaryArithmeticExpression>(
                BinaryArithmeticExpressionType::CONTAINS,
                Plan(expr.LeftChild()),
                std::make_unique<LiteralExpression>(match_info));
          }
        }

        // convert each paren into escape
        std::string regex = "^";
        for (auto c : match) {
          switch (c) {
            case '.':
            case '+':
            case '*':
            case '?':
            case '^':
            case '$':
            case '(':
            case ')':
            case '[':
            case ']':
            case '{':
            case '}':
            case '|':
            case '\\':
              regex.push_back('\\');
              regex.push_back(c);
              break;

            case '_':
              regex.push_back('.');
              break;

            case '%':
              regex.push_back('.');
              regex.push_back('*');
              break;

            default:
              regex.push_back(c);
              break;
          }
        }
        regex.push_back('$');

        return std::make_unique<RegexpMatchingExpression>(
            Plan(expr.LeftChild()), std::make_unique<re2::RE2>(regex));

      } else {
        throw std::runtime_error("Non const argument to like");
      }
    }
  }
}

std::unique_ptr<Expression> Planner::Plan(const parse::InExpression& expr) {
  std::unique_ptr<Expression> result;

  for (auto x : expr.Cases()) {
    auto left = Plan(expr.Base());
    auto right = Plan(x.get());

    std::unique_ptr<Expression> eq;
    if (left->Type().type_id == catalog::TypeId::ENUM) {
      if (auto r = dynamic_cast<kush::plan::LiteralExpression*>(right.get())) {
        std::string literal;
        r->Visit(
            nullptr, nullptr, nullptr, nullptr,
            [&](auto x, bool n) { literal = x; }, nullptr, nullptr, nullptr);
        auto r_rewrite =
            std::make_unique<LiteralExpression>(left->Type().enum_id, literal);
        eq = std::make_unique<kush::plan::BinaryArithmeticExpression>(
            kush::plan::BinaryArithmeticExpressionType::EQ, std::move(left),
            std::move(r_rewrite));
      }
    }

    if (eq == nullptr) {
      eq = std::make_unique<BinaryArithmeticExpression>(
          BinaryArithmeticExpressionType::EQ, std::move(left),
          std::move(right));
    }

    if (result == nullptr) {
      result = std::move(eq);
    } else {
      result = std::make_unique<BinaryArithmeticExpression>(
          BinaryArithmeticExpressionType::OR, std::move(result), std::move(eq));
    }
  }

  return result;
}

std::unique_ptr<Expression> Planner::Plan(const parse::CaseExpression& expr) {
  throw std::runtime_error("Unsupported case expression");
}

std::unique_ptr<Expression> Planner::Plan(
    const parse::ColumnRefExpression& expr) {
  std::pair<std::string, std::string> key;
  key.first = expr.TableName();
  key.second = expr.ColumnName();
  const auto& v = table_col_to_info_.at(key);
  return std::make_unique<ColumnRefExpression>(v.type, v.nullable, v.child_idx,
                                               v.col_idx);
}
std::unique_ptr<Expression> Planner::Plan(
    const parse::LiteralExpression& expr) {
  std::unique_ptr<Expression> result;
  expr.Visit(
      [&](int16_t arg) { result = std::make_unique<LiteralExpression>(arg); },
      [&](int32_t arg) { result = std::make_unique<LiteralExpression>(arg); },
      [&](int64_t arg) { result = std::make_unique<LiteralExpression>(arg); },
      [&](double arg) { result = std::make_unique<LiteralExpression>(arg); },
      [&](std::string arg) {
        result = std::make_unique<LiteralExpression>(arg);
      },
      [&](bool arg) { result = std::make_unique<LiteralExpression>(arg); });
  return result;
}

std::unique_ptr<Expression> Planner::Plan(const parse::Expression& expr) {
  if (auto v = dynamic_cast<const parse::AggregateExpression*>(&expr)) {
    return Plan(*v);
  }

  if (auto v = dynamic_cast<const parse::UnaryArithmeticExpression*>(&expr)) {
    return Plan(*v);
  }

  if (auto v = dynamic_cast<const parse::BinaryArithmeticExpression*>(&expr)) {
    return Plan(*v);
  }

  if (auto v = dynamic_cast<const parse::CaseExpression*>(&expr)) {
    return Plan(*v);
  }

  if (auto v = dynamic_cast<const parse::ColumnRefExpression*>(&expr)) {
    return Plan(*v);
  }

  if (auto v = dynamic_cast<const parse::InExpression*>(&expr)) {
    return Plan(*v);
  }

  if (auto v = dynamic_cast<const parse::LiteralExpression*>(&expr)) {
    return Plan(*v);
  }

  throw std::runtime_error("Unknown expression");
}

std::vector<std::unique_ptr<Expression>> Decompose(
    std::unique_ptr<Expression> e) {
  auto e_raw = e.get();
  if (auto v = dynamic_cast<BinaryArithmeticExpression*>(e_raw)) {
    if (v->OpType() == BinaryArithmeticExpressionType::AND) {
      std::vector<std::unique_ptr<Expression>> results;
      auto children = v->DestroyChildren();
      results = Decompose(std::move(children[0]));
      auto right = Decompose(std::move(children[1]));

      for (auto& right_expr : right) {
        results.push_back(std::move(right_expr));
      }

      return results;
    }
  }

  std::vector<std::unique_ptr<Expression>> result;
  result.push_back(std::move(e));
  return result;
}

void GetReferencedChildren(const Expression& expr,
                           std::unordered_set<int>& result) {
  if (auto v = dynamic_cast<const ColumnRefExpression*>(&expr)) {
    result.insert(v->GetChildIdx());
    return;
  }

  for (auto c : expr.Children()) {
    GetReferencedChildren(c.get(), result);
  }
}

std::unique_ptr<Operator> Planner::Plan(const parse::SelectStatement& stmt) {
  std::unique_ptr<Operator> result;
  {
    auto base_tables = Plan(stmt.From());
    std::vector<std::vector<std::unique_ptr<Expression>>> unary_preds_per_table(
        base_tables.size());
    std::vector<std::unique_ptr<Expression>> join_preds;

    auto exprs = Decompose(Plan(*stmt.Where()));
    for (auto& expr : exprs) {
      std::unordered_set<int> referenced_children;
      GetReferencedChildren(*expr, referenced_children);
      if (referenced_children.size() == 1) {
        // unary expression
        unary_preds_per_table[*referenced_children.begin()].push_back(
            std::move(expr));
      } else {
        join_preds.push_back(std::move(expr));
      }
    }

    // for each unary predicate, generate a scan + filter
    std::vector<std::unique_ptr<Operator>> input_tables;
    for (int i = 0; i < base_tables.size(); i++) {
      if (unary_preds_per_table[i].empty()) {
        input_tables.push_back(std::move(base_tables[i]));
        continue;
      }

      auto& preds = unary_preds_per_table[i];

      std::unique_ptr<Expression> filter = std::move(preds.back());

      for (int i = preds.size() - 2; i >= 0; i--) {
        filter = std::make_unique<BinaryArithmeticExpression>(
            BinaryArithmeticExpressionType::AND, std::move(preds[i]),
            std::move(filter));
      }

      // rewrite filter expression to set child idx to 0
      absl::flat_hash_set<std::pair<int, int>> refs;
      absl::flat_hash_map<std::pair<int, int>, std::pair<int, int>> rewrites;
      GetReferencedChildren(*filter, refs);
      for (auto ref : refs) {
        rewrites[ref] = {0, ref.second};
      }
      RewriteColumnReferences(*filter, rewrites);

      OperatorSchema schema;
      schema.AddPassthroughColumns(*base_tables[i]);
      input_tables.push_back(std::make_unique<SelectOperator>(
          std::move(schema), std::move(base_tables[i]), std::move(filter)));
    }

    absl::flat_hash_map<std::pair<int, int>, int> rewritten_child_col_idx;
    int output_col_idx = 0;
    for (int i = 0; i < input_tables.size(); i++) {
      for (int j = 0; j < input_tables[i]->Schema().Columns().size(); j++) {
        rewritten_child_col_idx[{i, j}] = output_col_idx++;
      }
    }

    // generate a skinner join with all the join predicates
    if (input_tables.size() == 1) {
      result = std::move(input_tables[0]);
    } else {
      OperatorSchema schema;
      int child_idx = 0;
      for (auto& v : input_tables) {
        schema.AddPassthroughColumns(*v, child_idx++);
      }
      result = std::make_unique<SkinnerJoinOperator>(
          std::move(schema), std::move(input_tables), std::move(join_preds));
    }

    {
      for (auto& [key, col_info] : table_col_to_info_) {
        col_info.col_idx =
            rewritten_child_col_idx.at({col_info.child_idx, col_info.col_idx});
        col_info.child_idx = 0;
      }
    }
  }

  // for now just no group by handling, only aggregates
  // additionally assume everything is aggregated
  OperatorSchema schema;
  int col_idx = 0;

  std::vector<std::unique_ptr<AggregateExpression>> aggs;
  for (auto select : stmt.Selects()) {
    auto agg = Plan(select.get());
    schema.AddDerivedColumn(std::to_string(col_idx),
                            std::make_unique<VirtualColumnRefExpression>(
                                agg->Type(), agg->Nullable(), col_idx));
    aggs.emplace_back(dynamic_cast<AggregateExpression*>(agg.release()));
    col_idx++;
  }

  return std::make_unique<OutputOperator>(std::make_unique<AggregateOperator>(
      std::move(schema), std::move(result), std::move(aggs)));
}

void EarlyProjection(Operator& op) {
  if (auto group_by = dynamic_cast<AggregateOperator*>(&op)) {
    // see which columns the group by references. delete the ones that we don't
    // and then rewrite.
    absl::flat_hash_set<std::pair<int, int>> refs;
    for (auto x : group_by->AggExprs()) {
      GetReferencedChildren(x.get(), refs);
    }

    absl::flat_hash_map<std::pair<int, int>, std::pair<int, int>>
        col_ref_to_rewrite_idx;
    int current_offset = 0;
    auto& child = group_by->Child();
    auto& child_schema = child.MutableSchema();
    auto& child_cols = child.Schema().Columns();
    for (int i = 0; i < child_cols.size(); i++) {
      if (refs.contains({0, i})) {
        col_ref_to_rewrite_idx[{0, i}] = {0, current_offset++};
        continue;
      }
    }

    for (int i = child_cols.size() - 1; i >= 0; i--) {
      if (!refs.contains({0, i})) {
        child_schema.RemoveColumn(i);
      }
    }

    for (auto x : group_by->MutableAggExprs()) {
      RewriteColumnReferences(x.get(), col_ref_to_rewrite_idx);
    }

    // recurse to child
    EarlyProjection(child);
    return;
  }

  if (auto join = dynamic_cast<SkinnerJoinOperator*>(&op)) {
    // collect the columns based on the output schema and join predicates
    absl::flat_hash_set<std::pair<int, int>> refs;
    for (auto& x : join->Schema().Columns()) {
      GetReferencedChildren(x.Expr(), refs);
    }
    for (auto x : join->Conditions()) {
      GetReferencedChildren(x.get(), refs);
    }

    absl::flat_hash_map<std::pair<int, int>, std::pair<int, int>>
        col_ref_to_rewrite_idx;
    int child_idx = 0;
    for (auto child : join->Children()) {
      int current_offset = 0;
      auto& child_schema = child.get().MutableSchema();
      auto& child_cols = child.get().Schema().Columns();
      for (int i = 0; i < child_cols.size(); i++) {
        if (refs.contains({child_idx, i})) {
          col_ref_to_rewrite_idx[{child_idx, i}] = {child_idx,
                                                    current_offset++};
          continue;
        }
      }

      for (int i = child_cols.size() - 1; i >= 0; i--) {
        if (!refs.contains({child_idx, i})) {
          child_schema.RemoveColumn(i);
        }
      }

      child_idx++;
    }

    for (auto& x : join->MutableSchema().MutableColumns()) {
      RewriteColumnReferences(x.MutableExpr(), col_ref_to_rewrite_idx);
    }
    for (auto x : join->MutableConditions()) {
      RewriteColumnReferences(x.get(), col_ref_to_rewrite_idx);
    }

    for (auto child : join->Children()) {
      EarlyProjection(child.get());
    }
    return;
  }

  if (auto select = dynamic_cast<SelectOperator*>(&op)) {
    // see which columns the group by references. delete the ones that we don't
    // and then rewrite.
    absl::flat_hash_set<std::pair<int, int>> refs;
    for (auto& x : select->Schema().Columns()) {
      GetReferencedChildren(x.Expr(), refs);
    }
    GetReferencedChildren(select->Expr(), refs);

    absl::flat_hash_map<std::pair<int, int>, std::pair<int, int>>
        col_ref_to_rewrite_idx;
    int current_offset = 0;
    auto& child = select->Child();
    auto& child_schema = child.MutableSchema();
    auto& child_cols = child.Schema().Columns();
    for (int i = 0; i < child_cols.size(); i++) {
      if (refs.contains({0, i})) {
        col_ref_to_rewrite_idx[{0, i}] = {0, current_offset++};
        continue;
      }
    }

    for (int i = child_cols.size() - 1; i >= 0; i--) {
      if (!refs.contains({0, i})) {
        child_schema.RemoveColumn(i);
      }
    }

    for (auto& x : select->MutableSchema().MutableColumns()) {
      RewriteColumnReferences(x.MutableExpr(), col_ref_to_rewrite_idx);
    }
    RewriteColumnReferences(select->MutableExpr(), col_ref_to_rewrite_idx);

    // recurse to child
    EarlyProjection(child);
    return;
  }

  if (auto output = dynamic_cast<OutputOperator*>(&op)) {
    EarlyProjection(output->Child());
    return;
  }

  if (auto scan = dynamic_cast<ScanOperator*>(&op)) {
    // get the referenced
    return;
  }

  throw std::runtime_error("Not supported.");
}

bool IsIndexFilter(Expression& e) {
  if (auto eq = dynamic_cast<BinaryArithmeticExpression*>(&e)) {
    if (eq->OpType() == BinaryArithmeticExpressionType::EQ) {
      if (auto l = dynamic_cast<ColumnRefExpression*>(&eq->LeftChild())) {
        if (auto r = dynamic_cast<LiteralExpression*>(&eq->RightChild())) {
          return true;
        }
      }

      if (auto l = dynamic_cast<LiteralExpression*>(&eq->LeftChild())) {
        if (auto r = dynamic_cast<ColumnRefExpression*>(&eq->RightChild())) {
          return true;
        }
      }
    }
  }
  return false;
}

void RewriteColRefToVirtColRef(std::unique_ptr<Expression>& e) {
  if (auto col_ref = dynamic_cast<ColumnRefExpression*>(e.get())) {
    e = std::make_unique<VirtualColumnRefExpression>(
        col_ref->Type(), col_ref->Nullable(), col_ref->GetColumnIdx());
    return;
  }

  auto children = e->DestroyChildren();
  for (auto& expr : children) {
    RewriteColRefToVirtColRef(expr);
  }
  e->SetChildren(std::move(children));
}

void AdaptiveScanSelect(std::unique_ptr<Operator>& op) {
  if (auto select = dynamic_cast<SelectOperator*>(op.get())) {
    if (auto scan = dynamic_cast<ScanOperator*>(&select->Children()[0].get())) {
      // scan / select
      auto exprs = Decompose(select->DestroyExpr());

      for (auto& expr : exprs) {
        RewriteColRefToVirtColRef(expr);
      }
      const auto& relation = scan->Relation();
      auto scan_schema = std::move(scan->MutableSchema());
      auto select_schema = std::move(select->MutableSchema());

      for (auto& col : select_schema.MutableColumns()) {
        auto e = col.DestroyExpr();
        RewriteColRefToVirtColRef(e);
        col.SetExpr(std::move(e));
      }

      op = std::make_unique<SkinnerScanSelectOperator>(
          std::move(select_schema), std::move(scan_schema), relation,
          std::move(exprs));

      return;
    }
  }

  for (auto& child : op->MutableChildren()) {
    AdaptiveScanSelect(child);
  }
}

std::unique_ptr<Operator> Planner::Plan(const parse::Statement& stmt) {
  if (auto v = dynamic_cast<const parse::SelectStatement*>(&stmt)) {
    auto result = Plan(*v);
    EarlyProjection(*result);
    AdaptiveScanSelect(result);
    return result;
  }

  throw std::runtime_error("Unknown statement");
}

}  // namespace kush::plan