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
#include "plan/order_by_operator.h"
#include "plan/output_operator.h"
#include "plan/scan_operator.h"
#include "plan/select_operator.h"
#include "tpch/schema.h"
#include "util/vector_util.h"

using namespace kush;
using namespace kush::plan;
using namespace kush::compile::cpp;
using namespace kush::catalog;

int main() {
  Database db = Schema();

  // scan(lineitem)
  std::unique_ptr<Operator> scan_lineitem;
  {
    OperatorSchema schema;
    schema.AddGeneratedColumns(
        db["lineitem"],
        {"l_returnflag", "l_linestatus", "l_quantity", "l_extendedprice",
         "l_discount", "l_tax", "l_shipdate"});
    scan_lineitem =
        std::make_unique<ScanOperator>(std::move(schema), "lineitem");
  }

  // Select (l_shipdate <= 1998-12-01)
  std::unique_ptr<Operator> select_lineitem;
  {
    auto l_shipdate = std::make_unique<ColumnRefExpression>(
        SqlType::DATE, 0, scan_lineitem->Schema().GetColumnIndex("l_shipdate"));
    auto literal =
        std::make_unique<LiteralExpression>(absl::CivilDay(1998, 12, 1));
    auto leq = std::make_unique<ComparisonExpression>(
        ComparisonType::LEQ, std::move(l_shipdate), std::move(literal));

    OperatorSchema schema;
    schema.AddPassthroughColumns(*scan_lineitem,
                                 {"l_returnflag", "l_linestatus", "l_quantity",
                                  "l_extendedprice", "l_discount", "l_tax"});

    select_lineitem = std::make_unique<SelectOperator>(
        std::move(schema), std::move(scan_lineitem), std::move(leq));
  }

  // Group By l_returnflag, l_linestatus -> Aggregate
  std::unique_ptr<Operator> agg;
  {
    // Group by
    std::unique_ptr<Expression> l_returnflag =
        std::make_unique<ColumnRefExpression>(
            SqlType::TEXT, 0,
            select_lineitem->Schema().GetColumnIndex("l_returnflag"));
    std::unique_ptr<Expression> l_linestatus =
        std::make_unique<ColumnRefExpression>(
            SqlType::TEXT, 0,
            select_lineitem->Schema().GetColumnIndex("l_linestatus"));

    // aggregate
    auto sum_qty = std::make_unique<AggregateExpression>(
        AggregateType::SUM,
        std::make_unique<ColumnRefExpression>(
            SqlType::REAL, 0,
            select_lineitem->Schema().GetColumnIndex("l_quantity")));

    auto sum_base_price = std::make_unique<AggregateExpression>(
        AggregateType::SUM,
        std::make_unique<ColumnRefExpression>(
            SqlType::REAL, 0,
            select_lineitem->Schema().GetColumnIndex("l_extendedprice")));

    auto sum_disc_price = std::make_unique<AggregateExpression>(
        AggregateType::SUM,
        std::make_unique<ArithmeticExpression>(
            ArithmeticOperatorType::MUL,
            std::make_unique<ColumnRefExpression>(
                SqlType::REAL, 0,
                select_lineitem->Schema().GetColumnIndex("l_extendedprice")),
            std::make_unique<ArithmeticExpression>(
                ArithmeticOperatorType::SUB,
                std::make_unique<LiteralExpression>(1),
                std::make_unique<ColumnRefExpression>(
                    SqlType::REAL, 0,
                    select_lineitem->Schema().GetColumnIndex("l_discount")))));

    auto sum_charge = std::make_unique<AggregateExpression>(
        AggregateType::SUM,
        std::make_unique<ArithmeticExpression>(
            ArithmeticOperatorType::MUL,
            std::make_unique<ArithmeticExpression>(
                ArithmeticOperatorType::MUL,
                std::make_unique<ColumnRefExpression>(
                    SqlType::REAL, 0,
                    select_lineitem->Schema().GetColumnIndex(
                        "l_extendedprice")),
                std::make_unique<ArithmeticExpression>(
                    ArithmeticOperatorType::SUB,
                    std::make_unique<LiteralExpression>(1),
                    std::make_unique<ColumnRefExpression>(
                        SqlType::REAL, 0,
                        select_lineitem->Schema().GetColumnIndex(
                            "l_discount")))),
            std::make_unique<ArithmeticExpression>(
                ArithmeticOperatorType::ADD,
                std::make_unique<LiteralExpression>(1),
                std::make_unique<ColumnRefExpression>(
                    SqlType::REAL, 0,
                    select_lineitem->Schema().GetColumnIndex("l_tax")))));

    auto avg_qty = std::make_unique<AggregateExpression>(
        AggregateType::AVG,
        std::make_unique<ColumnRefExpression>(
            SqlType::REAL, 0,
            select_lineitem->Schema().GetColumnIndex("l_quantity")));

    auto avg_price = std::make_unique<AggregateExpression>(
        AggregateType::AVG,
        std::make_unique<ColumnRefExpression>(
            SqlType::REAL, 0,
            select_lineitem->Schema().GetColumnIndex("l_extendedprice")));

    auto avg_disc = std::make_unique<AggregateExpression>(
        AggregateType::AVG,
        std::make_unique<ColumnRefExpression>(
            SqlType::REAL, 0,
            select_lineitem->Schema().GetColumnIndex("l_discount")));

    auto count_order = std::make_unique<AggregateExpression>(
        AggregateType::COUNT, std::make_unique<LiteralExpression>(true));

    // output
    OperatorSchema schema;
    schema.AddDerivedColumn(
        "l_returnflag",
        std::make_unique<VirtualColumnRefExpression>(SqlType::TEXT, 0));
    schema.AddDerivedColumn(
        "l_linestatus",
        std::make_unique<VirtualColumnRefExpression>(SqlType::TEXT, 1));
    schema.AddDerivedColumn(
        "sum_qty",
        std::make_unique<VirtualColumnRefExpression>(SqlType::REAL, 2));
    schema.AddDerivedColumn(
        "sum_base_price",
        std::make_unique<VirtualColumnRefExpression>(SqlType::REAL, 3));
    schema.AddDerivedColumn(
        "sum_disc_price",
        std::make_unique<VirtualColumnRefExpression>(SqlType::REAL, 4));
    schema.AddDerivedColumn(
        "sum_charge",
        std::make_unique<VirtualColumnRefExpression>(SqlType::REAL, 5));
    schema.AddDerivedColumn(
        "avg_qty",
        std::make_unique<VirtualColumnRefExpression>(SqlType::REAL, 6));
    schema.AddDerivedColumn(
        "avg_price",
        std::make_unique<VirtualColumnRefExpression>(SqlType::REAL, 7));
    schema.AddDerivedColumn(
        "avg_disc",
        std::make_unique<VirtualColumnRefExpression>(SqlType::REAL, 8));
    schema.AddDerivedColumn(
        "count_order",
        std::make_unique<VirtualColumnRefExpression>(SqlType::BIGINT, 9));

    agg = std::make_unique<GroupByAggregateOperator>(
        std::move(schema), std::move(select_lineitem),
        util::MakeVector(std::move(l_returnflag), std::move(l_linestatus)),
        util::MakeVector(std::move(sum_qty), std::move(sum_base_price),
                         std::move(sum_disc_price), std::move(sum_charge),
                         std::move(avg_qty), std::move(avg_price),
                         std::move(avg_disc), std::move(count_order)));
  }

  // Order By l_returnflag, l_linestatus
  std::unique_ptr<Operator> order;
  {
    std::unique_ptr<ColumnRefExpression> l_returnflag =
        std::make_unique<ColumnRefExpression>(
            SqlType::TEXT, 0, agg->Schema().GetColumnIndex("l_returnflag"));
    std::unique_ptr<ColumnRefExpression> l_linestatus =
        std::make_unique<ColumnRefExpression>(
            SqlType::TEXT, 0, agg->Schema().GetColumnIndex("l_linestatus"));

    OperatorSchema schema;
    schema.AddPassthroughColumns(*agg);

    order = std::make_unique<OrderByOperator>(
        std::move(schema), std::move(agg),
        util::MakeVector(std::move(l_returnflag), std::move(l_linestatus)),
        std::vector<bool>{true, true});
  }

  std::unique_ptr<Operator> query =
      std::make_unique<OutputOperator>(std::move(order));

  CppTranslator translator(db, *query);
  translator.Translate();
  auto& prog = translator.Program();
  prog.Compile();
  prog.Execute();
  return 0;
}
