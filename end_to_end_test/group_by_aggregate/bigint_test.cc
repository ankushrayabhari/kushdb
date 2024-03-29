#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "compile/query_translator.h"
#include "end_to_end_test/parameters.h"
#include "end_to_end_test/schema.h"
#include "end_to_end_test/test_macros.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/expression/arithmetic_expression.h"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/literal_expression.h"
#include "plan/expression/virtual_column_ref_expression.h"
#include "plan/operator/group_by_aggregate_operator.h"
#include "plan/operator/hash_join_operator.h"
#include "plan/operator/operator.h"
#include "plan/operator/operator_schema.h"
#include "plan/operator/order_by_operator.h"
#include "plan/operator/output_operator.h"
#include "plan/operator/scan_operator.h"
#include "plan/operator/select_operator.h"
#include "util/builder.h"
#include "util/test_util.h"

using namespace kush;
using namespace kush::util;
using namespace kush::plan;
using namespace kush::compile;
using namespace kush::catalog;
using namespace std::literals;

class GroupByAggregateTest : public testing::TestWithParam<ParameterValues> {};

TEST_P(GroupByAggregateTest, BigIntCol) {
  SetFlags(GetParam());

  auto db = Schema();

  std::unique_ptr<Operator> query;
  {
    std::unique_ptr<Operator> base;
    {
      OperatorSchema schema;
      schema.AddGeneratedColumns(db["info"], {"cheated", "num2"});
      base = std::make_unique<ScanOperator>(std::move(schema), db["info"]);
    }

    // Group By
    std::unique_ptr<Expression> cheated =
        Case(ColRefE(base, "cheated"), Literal(INT64_MAX), Literal(INT64_MIN));

    // Aggregate
    auto sum = Sum(ColRef(base, "num2"));
    auto min = Min(ColRef(base, "num2"));
    auto max = Max(ColRef(base, "num2"));
    auto avg = Avg(ColRef(base, "num2"));
    auto count1 = Count();
    auto count2 = Count(ColRef(base, "num2"));

    // output
    OperatorSchema schema;
    schema.AddDerivedColumn("sum", VirtColRef(sum, 1));
    schema.AddDerivedColumn("min", VirtColRef(min, 2));
    schema.AddDerivedColumn("max", VirtColRef(max, 3));
    schema.AddDerivedColumn("avg", VirtColRef(avg, 4));
    schema.AddDerivedColumn("count1", VirtColRef(count1, 5));
    schema.AddDerivedColumn("count2", VirtColRef(count2, 6));

    query = std::make_unique<OutputOperator>(
        std::make_unique<GroupByAggregateOperator>(
            std::move(schema), std::move(base),
            util::MakeVector(std::move(cheated)),
            util::MakeVector(std::move(sum), std::move(min), std::move(max),
                             std::move(avg), std::move(count1),
                             std::move(count2))));
  }

  auto expected_file = "end_to_end_test/group_by_aggregate/bigint_expected.tbl";
  auto output_file = ExecuteAndCapture(*query);

  auto expected = GetFileContents(expected_file);
  auto output = GetFileContents(output_file);
  std::sort(expected.begin(), expected.end());
  std::sort(output.begin(), output.end());
  EXPECT_EQ(output, expected);
}

NORMAL_TEST(GroupByAggregateTest)