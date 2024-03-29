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

TEST_P(GroupByAggregateTest, TextCol) {
  SetFlags(GetParam());

  auto db = EnumSchema();

  std::unique_ptr<Operator> query;
  {
    std::unique_ptr<Operator> base;
    {
      OperatorSchema schema;
      schema.AddGeneratedColumns(db["people"], {"name"});
      base = std::make_unique<ScanOperator>(std::move(schema), db["people"]);
    }

    // Group By
    std::unique_ptr<Expression> gp = ColRef(base, "name");

    // Aggregate
    auto min = Min(ColRef(base, "name"));
    auto max = Max(ColRef(base, "name"));
    auto count1 = Count();
    auto count2 = Count(ColRef(base, "name"));

    // output
    OperatorSchema schema;
    schema.AddDerivedColumn("min", VirtColRef(min, 1));
    schema.AddDerivedColumn("max", VirtColRef(max, 2));
    schema.AddDerivedColumn("count1", VirtColRef(count1, 3));
    schema.AddDerivedColumn("count2", VirtColRef(count2, 4));

    query = std::make_unique<OutputOperator>(
        std::make_unique<GroupByAggregateOperator>(
            std::move(schema), std::move(base), util::MakeVector(std::move(gp)),
            util::MakeVector(std::move(min), std::move(max), std::move(count1),
                             std::move(count2))));
  }

  auto expected_file = "end_to_end_test/group_by_aggregate/enum_expected.tbl";
  auto output_file = ExecuteAndCapture(*query);

  auto expected = GetFileContents(expected_file);
  auto output = GetFileContents(output_file);
  std::sort(expected.begin(), expected.end());
  std::sort(output.begin(), output.end());
  EXPECT_EQ(output, expected);
}

NORMAL_TEST(GroupByAggregateTest)