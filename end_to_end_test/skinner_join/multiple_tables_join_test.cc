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
  std::string skinner;
  int32_t budget_per_episode;
};

class SkinnerJoinTest : public testing::TestWithParam<ParameterValues> {};

ABSL_DECLARE_FLAG(std::string, backend);
ABSL_DECLARE_FLAG(std::string, skinner);
ABSL_DECLARE_FLAG(int32_t, budget_per_episode);

std::unique_ptr<Operator> Scan(const Database& db) {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["info"], {"id"});
  return std::make_unique<ScanOperator>(std::move(schema), db["info"]);
}

TEST_P(SkinnerJoinTest, IntCol) {
  auto params = GetParam();
  absl::SetFlag(&FLAGS_backend, params.backend);
  absl::SetFlag(&FLAGS_skinner, params.skinner);
  absl::SetFlag(&FLAGS_budget_per_episode, params.budget_per_episode);

  auto db = Schema();

  std::unique_ptr<Operator> query;
  {
    std::unique_ptr<Operator> s1 = Scan(db);
    std::unique_ptr<Operator> s2 = Scan(db);
    std::unique_ptr<Operator> s3 = Scan(db);
    std::unique_ptr<Operator> s4 = Scan(db);

    std::vector<std::unique_ptr<BinaryArithmeticExpression>> conditions;
    conditions.push_back(Eq(ColRef(s1, "id", 0), ColRef(s2, "id", 1)));
    conditions.push_back(Eq(ColRef(s2, "id", 1), ColRef(s3, "id", 2)));
    conditions.push_back(Eq(ColRef(s2, "id", 1), ColRef(s4, "id", 3)));

    OperatorSchema schema;
    schema.AddPassthroughColumns(*s1, 0);
    schema.AddPassthroughColumns(*s2, 1);
    schema.AddPassthroughColumns(*s3, 2);
    schema.AddPassthroughColumns(*s4, 3);
    query =
        std::make_unique<OutputOperator>(std::make_unique<SkinnerJoinOperator>(
            std::move(schema),
            util::MakeVector(std::move(s1), std::move(s2), std::move(s3),
                             std::move(s4)),
            std::move(conditions)));
  }

  auto expected_file =
      "end_to_end_test/skinner_join/multiple_tables_join_expected.tbl";
  auto output_file = ExecuteAndCapture(*query);

  auto expected = GetFileContents(expected_file);
  auto output = GetFileContents(output_file);
  std::sort(expected.begin(), expected.end());
  std::sort(output.begin(), output.end());
  EXPECT_EQ(output, expected);
}

INSTANTIATE_TEST_SUITE_P(ASMBackend_Recompile_HighBudget, SkinnerJoinTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .skinner = "recompile",
                             .budget_per_episode = 10000}));

INSTANTIATE_TEST_SUITE_P(
    ASMBackend_Permute_HighBudget, SkinnerJoinTest,
    testing::Values(ParameterValues{
        .backend = "asm", .skinner = "permute", .budget_per_episode = 10000}));

INSTANTIATE_TEST_SUITE_P(
    ASMBackend_Recompile_LowBudget, SkinnerJoinTest,
    testing::Values(ParameterValues{
        .backend = "asm", .skinner = "recompile", .budget_per_episode = 10}));

INSTANTIATE_TEST_SUITE_P(
    ASMBackend_Permute_LowBudget, SkinnerJoinTest,
    testing::Values(ParameterValues{
        .backend = "asm", .skinner = "permute", .budget_per_episode = 10}));

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