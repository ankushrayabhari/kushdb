#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include "absl/time/civil_time.h"
#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "compile/cpp_translator.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/expression/arithmetic_expression.h"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/comparison_expression.h"
#include "plan/expression/literal_expression.h"
#include "plan/expression/virtual_column_ref_expression.h"
#include "plan/group_by_aggregate_operator.h"
#include "plan/hash_join_operator.h"
#include "plan/operator.h"
#include "plan/operator_schema.h"
#include "plan/output_operator.h"
#include "plan/scan_operator.h"
#include "plan/select_operator.h"
#include "tpch/schema.h"
#include "util/vector_util.h"

using namespace kush;
using namespace kush::plan;
using namespace kush::compile;
using namespace kush::catalog;

int main() {
  Database db = Schema();

  // Scan(lineitem)
  std::unique_ptr<Operator> scan_lineitem;
  {
    OperatorSchema schema;
    schema.AddGeneratedColumns(db["lineitem"], {"l_extendedprice", "l_discount",
                                                "l_shipdate", "l_quantity"});
    scan_lineitem =
        std::make_unique<ScanOperator>(std::move(schema), "lineitem");
  }

  // Select(l_shipdate >= '1994-01-01' AND l_shipdate < '1995-01-01' AND
  // l_discount  >= 0.07 AND l_discount <= 0.09 AND l_quantity < 24)
  std::unique_ptr<Operator> select_lineitem;
  {
    auto p1_col = std::make_unique<ColumnRefExpression>(
        SqlType::DATE, 0, scan_lineitem->Schema().GetColumnIndex("l_shipdate"));
    auto p1_literal =
        std::make_unique<LiteralExpression>(absl::CivilDay(1994, 1, 1));
    auto p1 = std::make_unique<ComparisonExpression>(
        ComparisonType::GEQ, std::move(p1_col), std::move(p1_literal));

    auto p2_col = std::make_unique<ColumnRefExpression>(
        SqlType::DATE, 0, scan_lineitem->Schema().GetColumnIndex("l_shipdate"));
    auto p2_literal =
        std::make_unique<LiteralExpression>(absl::CivilDay(1995, 1, 1));
    auto p2 = std::make_unique<ComparisonExpression>(
        ComparisonType::LT, std::move(p2_col), std::move(p2_literal));

    auto p3_col = std::make_unique<ColumnRefExpression>(
        SqlType::REAL, 0, scan_lineitem->Schema().GetColumnIndex("l_discount"));
    auto p3_literal = std::make_unique<LiteralExpression>(0.07);
    auto p3 = std::make_unique<ComparisonExpression>(
        ComparisonType::GEQ, std::move(p3_col), std::move(p3_literal));

    auto p4_col = std::make_unique<ColumnRefExpression>(
        SqlType::REAL, 0, scan_lineitem->Schema().GetColumnIndex("l_discount"));
    auto p4_literal = std::make_unique<LiteralExpression>(0.09);
    auto p4 = std::make_unique<ComparisonExpression>(
        ComparisonType::LEQ, std::move(p4_col), std::move(p4_literal));

    auto p5_col = std::make_unique<ColumnRefExpression>(
        SqlType::REAL, 0, scan_lineitem->Schema().GetColumnIndex("l_quantity"));
    auto p5_literal = std::make_unique<LiteralExpression>(24);
    auto p5 = std::make_unique<ComparisonExpression>(
        ComparisonType::LT, std::move(p5_col), std::move(p5_literal));

    auto pred = std::make_unique<ComparisonExpression>(
        ComparisonType::AND,
        std::make_unique<ComparisonExpression>(
            ComparisonType::AND,
            std::make_unique<ComparisonExpression>(
                ComparisonType::AND,
                std::make_unique<ComparisonExpression>(
                    ComparisonType::AND, std::move(p1), std::move(p2)),
                std::move(p3)),
            std::move(p4)),
        std::move(p5));

    std::vector<std::string> columns{"l_extendedprice", "l_discount"};
    OperatorSchema schema;
    for (const auto& col : columns) {
      auto idx = scan_lineitem->Schema().GetColumnIndex(col);
      auto type = scan_lineitem->Schema().Columns()[idx].Expr().Type();
      schema.AddDerivedColumn(
          col, std::make_unique<ColumnRefExpression>(type, 0, idx));
    }
    select_lineitem = std::make_unique<SelectOperator>(
        std::move(schema), std::move(scan_lineitem), std::move(pred));
  }

  // Group By true -> AGG
  std::unique_ptr<Operator> agg;
  {
    // Group by
    std::unique_ptr<Expression> single_group =
        std::make_unique<LiteralExpression>(true);

    // aggregate
    auto revenue = std::make_unique<AggregateExpression>(
        AggregateType::SUM,
        std::make_unique<ArithmeticExpression>(
            ArithmeticOperatorType::MUL,
            std::make_unique<ColumnRefExpression>(
                SqlType::REAL, 0,
                select_lineitem->Schema().GetColumnIndex("l_extendedprice")),
            std::make_unique<ColumnRefExpression>(
                SqlType::REAL, 0,
                select_lineitem->Schema().GetColumnIndex("l_discount"))));

    OperatorSchema schema;
    schema.AddDerivedColumn(
        "revenue",
        std::make_unique<VirtualColumnRefExpression>(SqlType::REAL, 1));

    agg = std::make_unique<GroupByAggregateOperator>(
        std::move(schema), std::move(select_lineitem),
        util::MakeVector(std::move(single_group)),
        util::MakeVector(std::move(revenue)));
  }

  std::unique_ptr<Operator> query =
      std::make_unique<OutputOperator>(std::move(agg));

  CppTranslator translator(db, *query);
  translator.Translate();
  auto& prog = translator.Program();
  prog.Compile();
  prog.Execute();
  return 0;
}
