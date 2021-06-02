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

// rnspps JOIN subquery ON ps_supplycost = min_ps_supplycost
std::unique_ptr<Operator> Join() {
  auto r1 = ScanRegion();
  auto r2 = ScanRegion();

  std::vector<std::unique_ptr<BinaryArithmeticExpression>> conditions;
  conditions.push_back(Eq(ColRef(r1, "r_name", 0), ColRef(r2, "r_name", 1)));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*r1, {"r_regionkey", "r_name"}, 0);
  schema.AddPassthroughColumns(*r2, {"r_regionkey", "r_name"}, 1);
  return std::make_unique<SkinnerJoinOperator>(
      std::move(schema), util::MakeVector(std::move(r1), std::move(r2)),
      std::move(conditions));
}

int main() {
  std::unique_ptr<Operator> query = std::make_unique<OutputOperator>(Join());

  QueryTranslator translator(*query);
  auto prog = translator.Translate();
  prog->Execute();
  return 0;
}
