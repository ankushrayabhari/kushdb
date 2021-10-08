#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "compile/query_translator.h"
#include "end_to_end_test/execute_capture.h"
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

using namespace kush;
using namespace kush::util;
using namespace kush::plan;
using namespace kush::compile;
using namespace kush::catalog;
using namespace std::literals;

class SelectTest : public testing::TestWithParam<ParameterValues> {};

TEST_P(SelectTest, BigIntCol) {
  SetFlags(GetParam());

  auto db = Schema();

  std::unique_ptr<Operator> query;
  {
    std::unique_ptr<Operator> base;
    {
      OperatorSchema schema;
      schema.AddGeneratedColumns(db["info"], {"num2"});
      base = std::make_unique<ScanOperator>(std::move(schema), db["info"]);
    }

    auto filter = Gt(ColRef(base, "num2"), Literal(INT64_C(0)));

    // output
    OperatorSchema schema;
    schema.AddPassthroughColumns(*base);
    query = std::make_unique<OutputOperator>(std::make_unique<SelectOperator>(
        std::move(schema), std::move(base), std::move(filter)));
  }

  auto expected_file = "end_to_end_test/select/bigint_expected.tbl";
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