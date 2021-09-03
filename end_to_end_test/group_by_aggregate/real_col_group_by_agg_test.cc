#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "absl/flags/flag.h"

#include "gtest/gtest.h"

#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "compile/query_translator.h"
#include "end_to_end_test/execute_capture.h"
#include "end_to_end_test/schema.h"
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

using namespace kush;
using namespace kush::util;
using namespace kush::plan;
using namespace kush::compile;
using namespace kush::catalog;
using namespace std::literals;

struct ParameterValues {
  std::string backend;
};

class GroupByAggregateTest : public testing::TestWithParam<ParameterValues> {};

ABSL_DECLARE_FLAG(std::string, backend);

TEST_P(GroupByAggregateTest, RealCol) {
  auto params = GetParam();
  absl::SetFlag(&FLAGS_backend, params.backend);

  auto db = Schema();

  std::unique_ptr<Operator> query;
  {
    std::unique_ptr<Operator> base;
    {
      OperatorSchema schema;
      schema.AddGeneratedColumns(db["info"], {"zscore"});
      base = std::make_unique<ScanOperator>(std::move(schema), db["info"]);
    }

    // Aggregate
    auto sum = Sum(ColRef(base, "zscore"));
    auto min = Min(ColRef(base, "zscore"));
    auto max = Max(ColRef(base, "zscore"));
    auto avg = Avg(ColRef(base, "zscore"));
    auto count = Count();

    // output
    OperatorSchema schema;
    schema.AddDerivedColumn("sum", VirtColRef(sum, 0));
    schema.AddDerivedColumn("min", VirtColRef(min, 1));
    schema.AddDerivedColumn("max", VirtColRef(max, 2));
    schema.AddDerivedColumn("avg", VirtColRef(avg, 3));
    schema.AddDerivedColumn("count", VirtColRef(count, 4));

    query = std::make_unique<OutputOperator>(
        std::make_unique<GroupByAggregateOperator>(
            std::move(schema), std::move(base),
            std::vector<std::unique_ptr<Expression>>(),
            util::MakeVector(std::move(sum), std::move(min), std::move(max),
                             std::move(avg), std::move(count))));
  }

  auto expected_file =
      "end_to_end_test/group_by_aggregate/real_col_group_by_agg_expected.tbl";
  auto output_file = ExecuteAndCapture(*query);

  auto expected = GetFileContents(expected_file);
  auto output = GetFileContents(output_file);
  std::sort(expected.begin(), expected.end());
  std::sort(output.begin(), output.end());
  EXPECT_EQ(output, expected);
}

INSTANTIATE_TEST_SUITE_P(ASMBackend, GroupByAggregateTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                         }));

INSTANTIATE_TEST_SUITE_P(LLVMBackend, GroupByAggregateTest,
                         testing::Values(ParameterValues{
                             .backend = "llvm",
                         }));