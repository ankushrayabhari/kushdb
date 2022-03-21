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
#include "plan/operator/skinner_scan_select_operator.h"
#include "util/builder.h"
#include "util/test_util.h"

using namespace kush;
using namespace kush::util;
using namespace kush::plan;
using namespace kush::compile;
using namespace kush::catalog;
using namespace std::literals;

class SelectTest : public testing::TestWithParam<ParameterValues> {};

TEST_P(SelectTest, MultipleEqTest) {
  SetFlags(GetParam());

  auto db = Schema();

  std::unique_ptr<Operator> query;
  {
    OperatorSchema scan_schema;
    scan_schema.AddGeneratedColumns(db["info"], {"cheated", "date"});

    auto filter1 = Exp(Eq(VirtColRef(scan_schema, "cheated"), Literal(true)));

    auto filter2 = Exp(Eq(VirtColRef(scan_schema, "date"),
                          Literal(absl::CivilDay(2021, 1, 29))));

    OperatorSchema schema;
    schema.AddVirtualPassthroughColumns(scan_schema, {"cheated", "date"});
    query = std::make_unique<OutputOperator>(
        std::make_unique<SkinnerScanSelectOperator>(
            std::move(schema), std::move(scan_schema), db["info"],
            util::MakeVector(std::move(filter1), std::move(filter2))));
  }

  auto expected_file =
      "end_to_end_test/skinner_scan_select/multiple_eq_expected.tbl";
  auto output_file = ExecuteAndCapture(*query);

  auto expected = GetFileContents(expected_file);
  auto output = GetFileContents(output_file);
  std::sort(expected.begin(), expected.end());
  std::sort(output.begin(), output.end());

  EXPECT_EQ(output, expected);
}

INSTANTIATE_TEST_SUITE_P(ASMBackend_StackSpill_Seed1, SelectTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .reg_alloc = "stack_spill",
                             .scan_select_seed = 1337,
                         }));

INSTANTIATE_TEST_SUITE_P(ASMBackend_StackSpill_Seed2, SelectTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .reg_alloc = "stack_spill",
                             .scan_select_seed = 100,
                         }));

INSTANTIATE_TEST_SUITE_P(ASMBackend_StackSpill_Seed3, SelectTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .reg_alloc = "stack_spill",
                             .scan_select_seed = 420,
                         }));

INSTANTIATE_TEST_SUITE_P(ASMBackend_LinearScan_Seed1, SelectTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .reg_alloc = "linear_scan",
                             .scan_select_seed = 1337,
                         }));

INSTANTIATE_TEST_SUITE_P(ASMBackend_LinearScan_Seed2, SelectTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .reg_alloc = "linear_scan",
                             .scan_select_seed = 420,
                         }));

INSTANTIATE_TEST_SUITE_P(ASMBackend_LinearScan_Seed3, SelectTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .reg_alloc = "linear_scan",
                             .scan_select_seed = 100,
                         }));

INSTANTIATE_TEST_SUITE_P(LLVMBackend_Seed1, SelectTest,
                         testing::Values(ParameterValues{
                             .backend = "llvm",
                             .scan_select_seed = 1337,
                         }));

INSTANTIATE_TEST_SUITE_P(LLVMBackend_Seed2, SelectTest,
                         testing::Values(ParameterValues{
                             .backend = "llvm",
                             .scan_select_seed = 420,
                         }));

INSTANTIATE_TEST_SUITE_P(LLVMBackend_Seed3, SelectTest,
                         testing::Values(ParameterValues{
                             .backend = "llvm",
                             .scan_select_seed = 100,
                         }));

INSTANTIATE_TEST_SUITE_P(ASMBackend_StackSpill_Seed1_LowBudget, SelectTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .reg_alloc = "stack_spill",
                             .scan_select_seed = 1337,
                             .scan_select_budget_per_episode = 5,
                         }));

INSTANTIATE_TEST_SUITE_P(ASMBackend_StackSpill_Seed2_LowBudget, SelectTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .reg_alloc = "stack_spill",
                             .scan_select_seed = 100,
                             .scan_select_budget_per_episode = 5,
                         }));

INSTANTIATE_TEST_SUITE_P(ASMBackend_StackSpill_Seed3_LowBudget, SelectTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .reg_alloc = "stack_spill",
                             .scan_select_seed = 420,
                             .scan_select_budget_per_episode = 5,
                         }));

INSTANTIATE_TEST_SUITE_P(ASMBackend_LinearScan_Seed1_LowBudget, SelectTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .reg_alloc = "linear_scan",
                             .scan_select_seed = 1337,
                             .scan_select_budget_per_episode = 5,
                         }));

INSTANTIATE_TEST_SUITE_P(ASMBackend_LinearScan_Seed2_LowBudget, SelectTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .reg_alloc = "linear_scan",
                             .scan_select_seed = 420,
                             .scan_select_budget_per_episode = 5,
                         }));

INSTANTIATE_TEST_SUITE_P(ASMBackend_LinearScan_Seed3_LowBudget, SelectTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .reg_alloc = "linear_scan",
                             .scan_select_seed = 100,
                             .scan_select_budget_per_episode = 5,
                         }));

INSTANTIATE_TEST_SUITE_P(LLVMBackend_Seed1_LowBudget, SelectTest,
                         testing::Values(ParameterValues{
                             .backend = "llvm",
                             .scan_select_seed = 1337,
                             .scan_select_budget_per_episode = 5,
                         }));

INSTANTIATE_TEST_SUITE_P(LLVMBackend_Seed2_LowBudget, SelectTest,
                         testing::Values(ParameterValues{
                             .backend = "llvm",
                             .scan_select_seed = 420,
                             .scan_select_budget_per_episode = 5,
                         }));

INSTANTIATE_TEST_SUITE_P(LLVMBackend_Seed3_LowBudget, SelectTest,
                         testing::Values(ParameterValues{
                             .backend = "llvm",
                             .scan_select_seed = 100,
                             .scan_select_budget_per_episode = 5,
                         }));