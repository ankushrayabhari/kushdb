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

TEST_P(SelectTest, BooleanCol) {
  SetFlags(GetParam());

  auto db = Schema();

  std::unique_ptr<Operator> query;
  {
    OperatorSchema scan_schema;
    scan_schema.AddGeneratedColumns(db["info"], {"cheated"});

    std::unique_ptr<Expression> filter = VirtColRef(
        scan_schema.Columns()[scan_schema.GetColumnIndex("cheated")].Expr(), 0);

    // output
    OperatorSchema schema;
    schema.AddDerivedColumn(
        "cheated",
        VirtColRef(
            scan_schema.Columns()[scan_schema.GetColumnIndex("cheated")].Expr(),
            0));

    query = std::make_unique<OutputOperator>(
        std::make_unique<SkinnerScanSelectOperator>(
            std::move(schema), std::move(scan_schema), db["info"],
            util::MakeVector(std::move(filter))));
  }

  auto expected_file =
      "end_to_end_test/skinner_scan_select/boolean_expected.tbl";
  auto output_file = ExecuteAndCapture(*query);

  auto expected = GetFileContents(expected_file);
  auto output = GetFileContents(output_file);
  std::sort(expected.begin(), expected.end());
  std::sort(output.begin(), output.end());

  EXPECT_EQ(output, expected);
}

INSTANTIATE_TEST_SUITE_P(ASMBackend_StackSpill, SelectTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .reg_alloc = "stack_spill",
                         }));

INSTANTIATE_TEST_SUITE_P(ASMBackend_LinearScan, SelectTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .reg_alloc = "linear_scan",
                         }));

INSTANTIATE_TEST_SUITE_P(LLVMBackend, SelectTest,
                         testing::Values(ParameterValues{
                             .backend = "llvm",
                         }));