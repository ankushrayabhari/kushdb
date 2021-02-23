#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include "absl/time/civil_time.h"
#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "compile/llvm/llvm_ir.h"
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

const Database db = Schema();

// Scan(lineitem)
std::unique_ptr<Operator> ScanLineitem() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["nation"], {"n_nationkey"});
  return std::make_unique<ScanOperator>(std::move(schema), db["nation"]);
}

std::unique_ptr<Operator> OrderBy() {
  auto scan = ScanLineitem();

  auto l_quantity = ColRef(scan, "n_nationkey");

  OperatorSchema schema;
  schema.AddPassthroughColumns(*scan);

  return std::make_unique<OrderByOperator>(
      std::move(schema), std::move(scan),
      util::MakeVector(std::move(l_quantity)), std::vector<bool>{true});
}

std::unique_ptr<Operator> Join() {
  auto o1 = ScanLineitem();
  auto o2 = ScanLineitem();

  auto l_quantity_1 = ColRef(o1, "n_nationkey", 0);
  auto l_quantity_2 = ColRef(o2, "n_nationkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*o1, {"n_nationkey"}, 0);
  schema.AddPassthroughColumns(*o2, {"n_nationkey"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(o1), std::move(o2),
      util::MakeVector(std::move(l_quantity_1)),
      util::MakeVector(std::move(l_quantity_2)));
}

int main() {
  auto query = std::make_unique<OutputOperator>(Join());

  QueryTranslator translator(*query);
  auto prog = translator.Translate();
  prog->Compile();
  prog->Execute();

  return 0;
}
