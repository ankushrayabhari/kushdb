#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include "absl/time/civil_time.h"

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
#include "tpch/queries/builder.h"
#include "tpch/schema.h"
#include "util/vector_util.h"

using namespace kush;
using namespace kush::plan;
using namespace kush::compile;
using namespace kush::catalog;
using namespace std::literals;

const Database db = Schema();

// Scan(region)
std::unique_ptr<Operator> ScanRegion() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["region"], {"r_regionkey", "r_name"});
  return std::make_unique<ScanOperator>(std::move(schema), db["region"]);
}

// Group By l_returnflag, l_linestatus -> Aggregate
std::unique_ptr<Operator> GroupByAgg() {
  auto base = ScanRegion();

  // aggregate
  auto sum_regionkey = Sum(ColRef(base, "r_regionkey"));
  auto min_regionkey = Min(ColRef(base, "r_regionkey"));
  auto max_regionkey = Max(ColRef(base, "r_regionkey"));
  auto avg_regionkey = Avg(ColRef(base, "r_regionkey"));
  auto cnt = Count();

  // output
  OperatorSchema schema;
  schema.AddDerivedColumn("sum", VirtColRef(sum_regionkey, 1));
  schema.AddDerivedColumn("min", VirtColRef(min_regionkey, 2));
  schema.AddDerivedColumn("max", VirtColRef(max_regionkey, 3));
  schema.AddDerivedColumn("avg", VirtColRef(avg_regionkey, 4));
  schema.AddDerivedColumn("count", VirtColRef(cnt, 5));

  std::unique_ptr<Expression> asdf = Literal(1.0);

  return std::make_unique<GroupByAggregateOperator>(
      std::move(schema), std::move(base), util::MakeVector(std::move(asdf)),
      util::MakeVector(std::move(sum_regionkey), std::move(min_regionkey),
                       std::move(max_regionkey), std::move(avg_regionkey),
                       std::move(cnt)));
}

int main() {
  std::unique_ptr<Operator> query =
      std::make_unique<OutputOperator>(GroupByAgg());

  QueryTranslator translator(*query);
  auto prog = translator.Translate();
  prog->Execute();
  return 0;
}
