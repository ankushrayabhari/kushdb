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

class OrderByTest : public testing::TestWithParam<ParameterValues> {};

TEST_P(OrderByTest, TextCol) {
  auto params = GetParam();
  SetFlags(params);
  auto asc = params.asc;

  auto db = Schema();

  std::unique_ptr<Operator> query;
  {
    std::unique_ptr<Operator> base;
    {
      OperatorSchema schema;
      schema.AddGeneratedColumns(db["people"], {"id", "name"});
      base = std::make_unique<ScanOperator>(std::move(schema), db["people"]);
    }

    auto name = ColRef(base, "name");
    auto id = ColRef(base, "id");

    OperatorSchema schema;
    schema.AddPassthroughColumns(*base);
    query = std::make_unique<OutputOperator>(std::make_unique<OrderByOperator>(
        std::move(schema), std::move(base),
        util::MakeVector(std::move(name), std::move(id)),
        std::vector<bool>{asc, asc}));
  }

  auto expected_file = "end_to_end_test/order_by/text_expected.tbl";
  auto output_file = ExecuteAndCapture(*query);

  auto expected = GetFileContents(expected_file);
  auto output = GetFileContents(output_file);

  if (!asc) {
    std::reverse(expected.begin(), expected.end());
  }

  EXPECT_EQ(output, expected);
}

ORDER_TEST(OrderByTest)