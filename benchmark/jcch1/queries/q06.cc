#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/time/civil_time.h"

#include "benchmark/jcch1/schema.h"
#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "compile/query_translator.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/expression/binary_arithmetic_expression.h"
#include "plan/expression/column_ref_expression.h"
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
#include "util/builder.h"
#include "util/time_execute.h"
#include "util/vector_util.h"

using namespace kush;
using namespace kush::util;
using namespace kush::plan;
using namespace kush::compile;
using namespace kush::catalog;

const Database db = Schema();

// Scan(lineitem)
std::unique_ptr<Operator> ScanLineitem() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["lineitem"], {"l_extendedprice", "l_discount",
                                              "l_shipdate", "l_quantity"});
  return std::make_unique<ScanOperator>(std::move(schema), db["lineitem"]);
}

// Select(l_shipdate >= '1994-01-01' AND l_shipdate < '1995-01-01' AND
// l_discount  >= 0.02 AND l_discount <= 0.04 AND l_quantity < 25)
std::unique_ptr<Operator> SelectLineitem() {
  auto lineitem = ScanLineitem();

  std::unique_ptr<Expression> cond;
  {
    std::unique_ptr<Expression> p1 = Geq(ColRef(lineitem, "l_shipdate"),
                                         Literal(absl::CivilDay(1994, 1, 1)));
    std::unique_ptr<Expression> p2 =
        Lt(ColRef(lineitem, "l_shipdate"), Literal(absl::CivilDay(1995, 1, 1)));
    std::unique_ptr<Expression> p3 =
        Geq(ColRef(lineitem, "l_discount"), Literal(0.08));
    std::unique_ptr<Expression> p4 =
        Leq(ColRef(lineitem, "l_discount"), Literal(0.10));
    std::unique_ptr<Expression> p5 =
        Lt(ColRef(lineitem, "l_quantity"), Literal(100.0));

    cond = And(util::MakeVector(std::move(p1), std::move(p2), std::move(p3),
                                std::move(p4), std::move(p5)));
  }

  std::vector<std::string> ColumnRefExpression;
  OperatorSchema schema;
  schema.AddPassthroughColumns(*lineitem, {"l_extendedprice", "l_discount"});
  return std::make_unique<SelectOperator>(std::move(schema),
                                          std::move(lineitem), std::move(cond));
}

// Agg
std::unique_ptr<Operator> Agg() {
  auto lineitem = SelectLineitem();

  // aggregate
  auto revenue = Sum(
      Mul(ColRef(lineitem, "l_extendedprice"), ColRef(lineitem, "l_discount")));

  OperatorSchema schema;
  schema.AddDerivedColumn("revenue", VirtColRef(revenue, 0));

  return std::make_unique<GroupByAggregateOperator>(
      std::move(schema), std::move(lineitem),
      std::vector<std::unique_ptr<Expression>>(),
      util::MakeVector(std::move(revenue)));
}

int main(int argc, char** argv) {
  absl::SetProgramUsageMessage("Executes query.");
  absl::ParseCommandLine(argc, argv);
  auto query = std::make_unique<OutputOperator>(Agg());

  TimeExecute(*query);
  return 0;
}
