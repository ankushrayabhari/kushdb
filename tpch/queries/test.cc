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
std::unique_ptr<Operator> ScanNation() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["nation"], {"n_name"});
  return std::make_unique<ScanOperator>(std::move(schema), db["nation"]);
}

std::unique_ptr<Operator> OrderBy() {
  auto scan = ScanNation();

  auto n_name = ColRef(scan, "n_name");

  OperatorSchema schema;
  schema.AddPassthroughColumns(*scan);

  return std::make_unique<OrderByOperator>(std::move(schema), std::move(scan),
                                           util::MakeVector(std::move(n_name)),
                                           std::vector<bool>{true});
}

std::unique_ptr<Operator> Join() {
  auto o1 = OrderBy();
  auto o2 = OrderBy();

  auto n_name_1 = ColRef(o1, "n_name", 0);
  auto n_name_2 = ColRef(o2, "n_name", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*o1, {"n_name"}, 0);
  schema.AddPassthroughColumns(*o2, {"n_name"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(o1), std::move(o2),
      util::MakeVector(std::move(n_name_1)),
      util::MakeVector(std::move(n_name_2)));
}

int main() {
  auto query = std::make_unique<OutputOperator>(Join());

  QueryTranslator translator(*query);
  auto prog = translator.Translate();
  prog->Compile();
  prog->Execute();

  return 0;
}
