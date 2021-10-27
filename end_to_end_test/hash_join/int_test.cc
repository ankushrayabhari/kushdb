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
#include "plan/expression/aggregate_expression.h"
#include "plan/expression/binary_arithmetic_expression.h"
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

class HashJoinTest : public testing::TestWithParam<ParameterValues> {};

TEST_P(HashJoinTest, IntCol) {
  SetFlags(GetParam());

  auto db = Schema();

  std::unique_ptr<Operator> query;
  {
    std::unique_ptr<Operator> s1;
    {
      OperatorSchema schema;
      schema.AddGeneratedColumns(db["people"], {"id"});
      s1 = std::make_unique<ScanOperator>(std::move(schema), db["people"]);
    }

    std::unique_ptr<Operator> s2;
    {
      OperatorSchema schema;
      schema.AddGeneratedColumns(db["info"], {"id"});
      s2 = std::make_unique<ScanOperator>(std::move(schema), db["info"]);
    }

    auto col1 = ColRef(s1, "id", 0);
    auto col2 = ColRef(s2, "id", 1);

    OperatorSchema schema;
    schema.AddPassthroughColumns(*s1, 0);
    schema.AddPassthroughColumns(*s2, 1);
    query = std::make_unique<OutputOperator>(std::make_unique<HashJoinOperator>(
        std::move(schema), std::move(s1), std::move(s2),
        util::MakeVector(std::move(col1)), util::MakeVector(std::move(col2))));
  }

  auto expected_file = "end_to_end_test/hash_join/int_expected.tbl";
  auto output_file = ExecuteAndCapture(*query);

  auto expected = GetFileContents(expected_file);
  auto output = GetFileContents(output_file);
  std::sort(expected.begin(), expected.end());
  std::sort(output.begin(), output.end());
  EXPECT_EQ(output, expected);
}

INSTANTIATE_TEST_SUITE_P(ASMBackend_StackSpill, HashJoinTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .reg_alloc = "stack_spill",
                         }));

INSTANTIATE_TEST_SUITE_P(ASMBackend_LinearScan, HashJoinTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .reg_alloc = "linear_scan",
                         }));

INSTANTIATE_TEST_SUITE_P(LLVMBackend, HashJoinTest,
                         testing::Values(ParameterValues{.backend = "llvm"}));