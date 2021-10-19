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
#include "end_to_end_test/test_util.h"
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

class OrderByTest
    : public testing::TestWithParam<std::pair<ParameterValues, bool>> {};

TEST_P(OrderByTest, BigIntCol) {
  auto [params, asc] = GetParam();
  SetFlags(params);

  auto db = Schema();

  std::unique_ptr<Operator> query;
  {
    std::unique_ptr<Operator> base;
    {
      OperatorSchema schema;
      schema.AddGeneratedColumns(db["info"], {"num2", "id"});
      base = std::make_unique<ScanOperator>(std::move(schema), db["info"]);
    }

    auto num2 = ColRef(base, "num2");
    auto id = ColRef(base, "id");

    OperatorSchema schema;
    schema.AddPassthroughColumns(*base);
    query = std::make_unique<OutputOperator>(std::make_unique<OrderByOperator>(
        std::move(schema), std::move(base),
        util::MakeVector(std::move(num2), std::move(id)),
        std::vector<bool>{asc, asc}));
  }

  auto expected_file = "end_to_end_test/order_by/bigint_expected.tbl";
  auto output_file = ExecuteAndCapture(*query);

  auto expected = GetFileContents(expected_file);
  auto output = GetFileContents(output_file);

  if (!asc) {
    std::reverse(expected.begin(), expected.end());
  }

  EXPECT_EQ(output, expected);
}

INSTANTIATE_TEST_SUITE_P(ASMBackend_Asc_StackSpill, OrderByTest,
                         testing::Values(std::make_pair(
                             ParameterValues{
                                 .backend = "asm",
                                 .reg_alloc = "stack_spill",
                             },
                             true)));

INSTANTIATE_TEST_SUITE_P(ASMBackend_Desc_StackSpill, OrderByTest,
                         testing::Values(std::make_pair(
                             ParameterValues{
                                 .backend = "asm",
                                 .reg_alloc = "stack_spill",
                             },
                             false)));

INSTANTIATE_TEST_SUITE_P(ASMBackend_Asc_LinearScan, OrderByTest,
                         testing::Values(std::make_pair(
                             ParameterValues{
                                 .backend = "asm",
                                 .reg_alloc = "linear_scan",
                             },
                             true)));

INSTANTIATE_TEST_SUITE_P(ASMBackend_Desc_LinearScan, OrderByTest,
                         testing::Values(std::make_pair(
                             ParameterValues{
                                 .backend = "asm",
                                 .reg_alloc = "linear_scan",
                             },
                             false)));

INSTANTIATE_TEST_SUITE_P(LLVMBackend_Asc, OrderByTest,
                         testing::Values(std::make_pair(
                             ParameterValues{
                                 .backend = "llvm",
                             },
                             true)));

INSTANTIATE_TEST_SUITE_P(LLVMBackend_Desc, OrderByTest,
                         testing::Values(std::make_pair(
                             ParameterValues{
                                 .backend = "llvm",
                             },
                             false)));