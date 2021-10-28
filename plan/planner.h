#pragma once

#include "absl/container/flat_hash_map.h"

#include "catalog/sql_type.h"
#include "parse/expression/aggregate_expression.h"
#include "parse/expression/arithmetic_expression.h"
#include "parse/expression/case_expression.h"
#include "parse/expression/column_ref_expression.h"
#include "parse/expression/expression.h"
#include "parse/expression/in_expression.h"
#include "parse/expression/literal_expression.h"
#include "parse/statement/select_statement.h"
#include "parse/table/table.h"
#include "plan/operator/operator.h"

namespace kush::plan {

class Planner {
 public:
  std::unique_ptr<Operator> Plan(const parse::SelectStatement& stmt);

 private:
  struct ColumnInfo {
    int child_idx, col_idx;
    catalog::SqlType type;
    bool nullable;
  };
  absl::flat_hash_map<std::pair<std::string, std::string>, ColumnInfo>
      table_col_to_info_;

  std::unique_ptr<Operator> Plan(const parse::Table& table, int child_idx = 0);
  std::unique_ptr<Expression> Plan(const parse::AggregateExpression& expr);
  std::unique_ptr<Expression> Plan(
      const parse::UnaryArithmeticExpression& expr);
  std::unique_ptr<Expression> Plan(
      const parse::BinaryArithmeticExpression& expr);
  std::unique_ptr<Expression> Plan(const parse::CaseExpression& expr);
  std::unique_ptr<Expression> Plan(const parse::ColumnRefExpression& expr);
  std::unique_ptr<Expression> Plan(const parse::InExpression& expr);
  std::unique_ptr<Expression> Plan(const parse::LiteralExpression& expr);
  std::unique_ptr<Expression> Plan(const parse::Expression& expr);
};

}  // namespace kush::plan