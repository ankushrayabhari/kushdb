#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include "absl/time/civil_time.h"
#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "compile/cpp/cpp_translator.h"
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
    schema.AddGeneratedColumns(db["lineitem"], {"l_shipmode", "l_commitdate",
                                                "l_shipdate", "l_receiptdate"});
    scan_lineitem =
        std::make_unique<ScanOperator>(std::move(schema), "lineitem");
  }

  // Select((l_shipmode = 'FOB' OR l_shipmode = 'SHIP') AND l_commitdate <
  // l_receiptdate AND l_shipdate < l_commitdate AND l_receiptdate >=
  // '1994-01-01' and l_receiptdate < '1995-01-01')
  std::unique_ptr<Operator> select_lineitem;
  {
    auto p1_col = std::make_unique<ColumnRefExpression>(
        SqlType::TEXT, 0, scan_lineitem->Schema().GetColumnIndex("l_shipmode"));
    auto p1_literal = std::make_unique<LiteralExpression>("FOB");
    auto p1 = std::make_unique<ComparisonExpression>(
        ComparisonType::EQ, std::move(p1_col), std::move(p1_literal));

    auto p2_col = std::make_unique<ColumnRefExpression>(
        SqlType::TEXT, 0, scan_lineitem->Schema().GetColumnIndex("l_shipmode"));
    auto p2_literal = std::make_unique<LiteralExpression>("SHIP");
    auto p2 = std::make_unique<ComparisonExpression>(
        ComparisonType::EQ, std::move(p2_col), std::move(p2_literal));

    auto p3_col1 = std::make_unique<ColumnRefExpression>(
        SqlType::DATE, 0,
        scan_lineitem->Schema().GetColumnIndex("l_commitdate"));
    auto p3_col2 = std::make_unique<ColumnRefExpression>(
        SqlType::DATE, 0,
        scan_lineitem->Schema().GetColumnIndex("l_receiptdate"));
    auto p3 = std::make_unique<ComparisonExpression>(
        ComparisonType::LT, std::move(p3_col1), std::move(p3_col2));

    auto p4_col1 = std::make_unique<ColumnRefExpression>(
        SqlType::DATE, 0, scan_lineitem->Schema().GetColumnIndex("l_shipdate"));
    auto p4_col2 = std::make_unique<ColumnRefExpression>(
        SqlType::DATE, 0,
        scan_lineitem->Schema().GetColumnIndex("l_commitdate"));
    auto p4 = std::make_unique<ComparisonExpression>(
        ComparisonType::LT, std::move(p4_col1), std::move(p4_col2));

    auto p5_col = std::make_unique<ColumnRefExpression>(
        SqlType::DATE, 0,
        scan_lineitem->Schema().GetColumnIndex("l_receiptdate"));
    auto p5_literal =
        std::make_unique<LiteralExpression>(absl::CivilDay(1994, 1, 1));
    auto p5 = std::make_unique<ComparisonExpression>(
        ComparisonType::GEQ, std::move(p5_col), std::move(p5_literal));

    auto p6_col = std::make_unique<ColumnRefExpression>(
        SqlType::DATE, 0,
        scan_lineitem->Schema().GetColumnIndex("l_receiptdate"));
    auto p6_literal =
        std::make_unique<LiteralExpression>(absl::CivilDay(1995, 1, 1));
    auto p6 = std::make_unique<ComparisonExpression>(
        ComparisonType::LT, std::move(p6_col), std::move(p6_literal));

    auto pred = std::make_unique<ComparisonExpression>(
        ComparisonType::AND,
        std::make_unique<ComparisonExpression>(
            ComparisonType::AND,
            std::make_unique<ComparisonExpression>(
                ComparisonType::AND,
                std::make_unique<ComparisonExpression>(
                    ComparisonType::AND,
                    std::make_unique<ComparisonExpression>(
                        ComparisonType::OR, std::move(p1), std::move(p2)),
                    std::move(p3)),
                std::move(p4)),
            std::move(p5)),
        std::move(p6));

    OperatorSchema schema;
    schema.AddPassthroughColumns(*scan_lineitem, {"l_shipmode"});
    select_lineitem = std::make_unique<SelectOperator>(
        std::move(schema), std::move(scan_lineitem), std::move(pred));
  }

  // Group By l_shipmode -> l_shipmode
  std::unique_ptr<Operator> agg;
  {
    // Group by
    std::unique_ptr<Expression> single_group =
        std::make_unique<ColumnRefExpression>(
            SqlType::TEXT, 0,
            select_lineitem->Schema().GetColumnIndex("l_shipmode"));

    OperatorSchema schema;
    schema.AddDerivedColumn(
        "l_shipmode",
        std::make_unique<VirtualColumnRefExpression>(SqlType::TEXT, 0));

    agg = std::make_unique<GroupByAggregateOperator>(
        std::move(schema), std::move(select_lineitem),
        util::MakeVector(std::move(single_group)),
        util::MakeVector<std::unique_ptr<AggregateExpression>>());
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
