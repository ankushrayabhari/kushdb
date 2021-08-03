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

std::unique_ptr<Operator> Scan() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["region"], {"r_regionkey", "r_name"});
  return std::make_unique<ScanOperator>(std::move(schema), db["region"]);
}

std::unique_ptr<Operator> Select() {
  auto scan = Scan();

  auto geq = std::make_unique<BinaryArithmeticExpression>(
      BinaryArithmeticOperatorType::GEQ, ColRef(scan, "r_regionkey"),
      Literal(1));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*scan, {"r_regionkey", "r_name"});

  return std::make_unique<SelectOperator>(std::move(schema), std::move(scan),
                                          std::move(geq));
}

// Order By l_returnflag, l_linestatus
std::unique_ptr<Operator> OrderBy() {
  auto base = Select();

  auto r_name = ColRef(base, "r_name");

  OperatorSchema schema;
  schema.AddPassthroughColumns(*base);

  return std::make_unique<OrderByOperator>(std::move(schema), std::move(base),
                                           util::MakeVector(std::move(r_name)),
                                           std::vector<bool>{true});
}

std::unique_ptr<Operator> HashJoin() {
  auto r1 = OrderBy();
  auto r2 = Scan();

  auto c1 = ColRef(r1, "r_name", 0);
  auto c2 = ColRef(r2, "r_name", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*r1, {"r_regionkey", "r_name"}, 0);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(r1), std::move(r2),
      util::MakeVector(std::move(c1)), util::MakeVector(std::move(c2)));
}

std::unique_ptr<Operator> SkinnerJoin() {
  auto r1 = OrderBy();
  auto r2 = Scan();

  std::vector<std::unique_ptr<BinaryArithmeticExpression>> conditions;
  conditions.push_back(Eq(ColRef(r1, "r_name", 0), ColRef(r2, "r_name", 1)));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*r1, {"r_regionkey", "r_name"}, 0);
  return std::make_unique<SkinnerJoinOperator>(
      std::move(schema), util::MakeVector(std::move(r1), std::move(r2)),
      std::move(conditions));
}

std::unique_ptr<Operator> GroupByAgg(std::unique_ptr<Operator> base) {
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
  {
    std::unique_ptr<Operator> query =
        std::make_unique<OutputOperator>(GroupByAgg(SkinnerJoin()));

    QueryTranslator translator(*query);
    auto prog = translator.Translate();
    prog->Execute();
  }

  {
    std::unique_ptr<Operator> query =
        std::make_unique<OutputOperator>(GroupByAgg(HashJoin()));

    QueryTranslator translator(*query);
    auto prog = translator.Translate();
    prog->Execute();
  }
  return 0;
}
