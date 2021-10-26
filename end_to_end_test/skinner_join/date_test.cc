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
#include "plan/group_by_aggregate_operator.h"
#include "plan/hash_join_operator.h"
#include "plan/operator.h"
#include "plan/operator_schema.h"
#include "plan/order_by_operator.h"
#include "plan/output_operator.h"
#include "plan/scan_operator.h"
#include "plan/select_operator.h"
#include "util/builder.h"
#include "util/test_util.h"

using namespace kush;
using namespace kush::util;
using namespace kush::plan;
using namespace kush::compile;
using namespace kush::catalog;
using namespace std::literals;

class SkinnerJoinTest : public testing::TestWithParam<ParameterValues> {};

TEST_P(SkinnerJoinTest, DateCol) {
  SetFlags(GetParam());

  auto db = Schema();

  std::unique_ptr<Operator> query;
  {
    std::unique_ptr<Operator> s1;
    {
      OperatorSchema schema;
      schema.AddGeneratedColumns(db["info"], {"date"});
      s1 = std::make_unique<ScanOperator>(std::move(schema), db["info"]);
    }

    std::unique_ptr<Operator> s2;
    {
      OperatorSchema schema;
      schema.AddGeneratedColumns(db["info"], {"date"});
      s2 = std::make_unique<ScanOperator>(std::move(schema), db["info"]);
    }

    std::vector<std::unique_ptr<BinaryArithmeticExpression>> conditions;
    conditions.push_back(Eq(ColRef(s1, "date", 0), ColRef(s2, "date", 1)));

    OperatorSchema schema;
    schema.AddPassthroughColumns(*s1, 0);
    schema.AddPassthroughColumns(*s2, 1);
    query =
        std::make_unique<OutputOperator>(std::make_unique<SkinnerJoinOperator>(
            std::move(schema), util::MakeVector(std::move(s1), std::move(s2)),
            std::move(conditions)));
  }

  auto expected_file = "end_to_end_test/skinner_join/date_expected.tbl";
  auto output_file = ExecuteAndCapture(*query);

  auto expected = GetFileContents(expected_file);
  auto output = GetFileContents(output_file);
  std::sort(expected.begin(), expected.end());
  std::sort(output.begin(), output.end());
  EXPECT_EQ(output, expected);
}

INSTANTIATE_TEST_SUITE_P(ASMBackend_StackSpill_Recompile_HighBudget,
                         SkinnerJoinTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .reg_alloc = "stack_spill",
                             .skinner = "recompile",
                             .budget_per_episode = 10000}));

INSTANTIATE_TEST_SUITE_P(ASMBackend_StackSpill_Permute_HighBudget,
                         SkinnerJoinTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .reg_alloc = "stack_spill",
                             .skinner = "permute",
                             .budget_per_episode = 10000}));

INSTANTIATE_TEST_SUITE_P(ASMBackend_StackSpill_Recompile_LowBudget,
                         SkinnerJoinTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .reg_alloc = "stack_spill",
                             .skinner = "recompile",
                             .budget_per_episode = 10}));

INSTANTIATE_TEST_SUITE_P(ASMBackend_StackSpill_Permute_LowBudget,
                         SkinnerJoinTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .reg_alloc = "stack_spill",
                             .skinner = "permute",
                             .budget_per_episode = 10}));

INSTANTIATE_TEST_SUITE_P(ASMBackend_LinearScan_Recompile_HighBudget,
                         SkinnerJoinTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .reg_alloc = "linear_scan",
                             .skinner = "recompile",
                             .budget_per_episode = 10000}));

INSTANTIATE_TEST_SUITE_P(ASMBackend_LinearScan_Permute_HighBudget,
                         SkinnerJoinTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .reg_alloc = "linear_scan",
                             .skinner = "permute",
                             .budget_per_episode = 10000}));

INSTANTIATE_TEST_SUITE_P(ASMBackend_LinearScan_Recompile_LowBudget,
                         SkinnerJoinTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .reg_alloc = "linear_scan",
                             .skinner = "recompile",
                             .budget_per_episode = 10}));

INSTANTIATE_TEST_SUITE_P(ASMBackend_LinearScan_Permute_LowBudget,
                         SkinnerJoinTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .reg_alloc = "linear_scan",
                             .skinner = "permute",
                             .budget_per_episode = 10}));

INSTANTIATE_TEST_SUITE_P(LLVMBackend_Recompile_HighBudget, SkinnerJoinTest,
                         testing::Values(ParameterValues{
                             .backend = "llvm",
                             .skinner = "recompile",
                             .budget_per_episode = 10000}));

INSTANTIATE_TEST_SUITE_P(
    LLVMBackend_Permute_HighBudget, SkinnerJoinTest,
    testing::Values(ParameterValues{
        .backend = "llvm", .skinner = "permute", .budget_per_episode = 10000}));

INSTANTIATE_TEST_SUITE_P(
    LLVMBackend_Recompile_LowBudget, SkinnerJoinTest,
    testing::Values(ParameterValues{
        .backend = "llvm", .skinner = "recompile", .budget_per_episode = 10}));

INSTANTIATE_TEST_SUITE_P(
    LLVMBackend_Permute_LowBudget, SkinnerJoinTest,
    testing::Values(ParameterValues{
        .backend = "llvm", .skinner = "permute", .budget_per_episode = 10}));