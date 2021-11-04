#include "plan/planner.h"

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
#include "plan/operator/group_by_aggregate_operator.h"
#include "plan/operator/operator.h"
#include "plan/operator/scan_operator.h"
#include "plan/operator/select_operator.h"
#include "plan/operator/skinner_join_operator.h"
#include "util/vector_util.h"

namespace kush::plan {

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
      v.type = col.get().Type();
      v.nullable = col.get().Nullable();
    }

    OperatorSchema schema;
    schema.AddGeneratedColumns(table);
    std::unique_ptr<Operator> op =
        std::make_unique<plan::ScanOperator>(std::move(schema), table);
    return util::MakeVector(std::move(op));
  }

  if (auto cross_product =
          dynamic_cast<const parse::CrossProductTable*>(&table)) {
    OperatorSchema schema;
    std::vector<std::unique_ptr<Operator>> children;

    int child_idx = 0;
    for (auto table : cross_product->Children()) {
      for (auto& x : Plan(table.get(), child_idx)) {
        children.push_back(std::move(x));
      }
      schema.AddPassthroughColumns(*children.back(), child_idx);
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

    case parse::BinaryArithmeticExpressionType::EQ:
      return std::make_unique<BinaryArithmeticExpression>(
          BinaryArithmeticExpressionType::EQ, Plan(expr.LeftChild()),
          Plan(expr.RightChild()));

    case parse::BinaryArithmeticExpressionType::NEQ:
      return std::make_unique<BinaryArithmeticExpression>(
          BinaryArithmeticExpressionType::NEQ, Plan(expr.LeftChild()),
          Plan(expr.RightChild()));

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
        std::string regex;
        for (auto c : match) {
          switch (c) {
            case '(':
              regex.push_back('\\');
              regex.push_back('(');
              break;
            case ')':
              regex.push_back('\\');
              regex.push_back(')');
              break;
            case '+':
              regex.push_back('\\');
              regex.push_back('+');
              break;
            case '*':
              regex.push_back('\\');
              regex.push_back('*');
              break;
            case '?':
              regex.push_back('\\');
              regex.push_back('?');
              break;
            case '|':
              regex.push_back('\\');
              regex.push_back('|');
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

        return std::make_unique<BinaryArithmeticExpression>(
            BinaryArithmeticExpressionType::LIKE, Plan(expr.LeftChild()),
            std::make_unique<LiteralExpression>(regex));

      } else {
        throw std::runtime_error("Non const argument to like");
      }
    }
  }
}

std::unique_ptr<Expression> Planner::Plan(const parse::InExpression& expr) {
  std::unique_ptr<Expression> result;

  for (auto x : expr.Cases()) {
    auto eq = std::make_unique<BinaryArithmeticExpression>(
        BinaryArithmeticExpressionType::EQ, Plan(expr.Base()), Plan(x.get()));

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
      [&](bool arg) { result = std::make_unique<LiteralExpression>(arg); },
      [&](absl::CivilDay arg) {
        result = std::make_unique<LiteralExpression>(arg);
      });
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
      auto children = v->DestroyAndGetChildren();
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
  if (auto v = dynamic_cast<const plan::ColumnRefExpression*>(&expr)) {
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

      OperatorSchema schema;
      schema.AddPassthroughColumns(*base_tables[i]);
      input_tables.push_back(std::make_unique<SelectOperator>(
          std::move(schema), std::move(base_tables[i]), std::move(filter)));
    }

    // generate a skinner join with all the join predicates
    if (input_tables.size() == 1) {
      result = std::move(input_tables[0]);
    } else {
      OperatorSchema schema;
      for (auto& v : input_tables) {
        schema.AddPassthroughColumns(*v);
      }
      result = std::make_unique<SkinnerJoinOperator>(
          std::move(schema), std::move(input_tables), std::move(join_preds));
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
  }

  return std::make_unique<GroupByAggregateOperator>(
      std::move(schema), std::move(result),
      std::vector<std::unique_ptr<Expression>>(), std::move(aggs));
}

std::unique_ptr<Operator> Planner::Plan(const parse::Statement& stmt) {
  if (auto v = dynamic_cast<const parse::SelectStatement*>(&stmt)) {
    return Plan(*v);
  }

  throw std::runtime_error("Unknown statement");
}

}  // namespace kush::plan