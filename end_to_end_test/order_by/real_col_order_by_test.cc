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
  bool asc;
};

class OrderByTest : public testing::TestWithParam<ParameterValues> {};

ABSL_DECLARE_FLAG(std::string, backend);

TEST_P(OrderByTest, RealCol) {
  auto params = GetParam();
  absl::SetFlag(&FLAGS_backend, params.backend);

  auto db = Schema();

  std::unique_ptr<Operator> query;
  {
    std::unique_ptr<Operator> base;
    {
      OperatorSchema schema;
      schema.AddGeneratedColumns(db["info"], {"zscore", "id"});
      base = std::make_unique<ScanOperator>(std::move(schema), db["info"]);
    }

    // Aggregate
    auto zscore = ColRef(base, "zscore");
    auto id = ColRef(base, "id");

    // output
    OperatorSchema schema;
    schema.AddPassthroughColumns(*base);
    query = std::make_unique<OutputOperator>(std::make_unique<OrderByOperator>(
        std::move(schema), std::move(base),
        util::MakeVector(std::move(zscore), std::move(id)),
        std::vector<bool>{params.asc, params.asc}));
  }

  auto expected_file =
      "end_to_end_test/order_by/real_col_order_by_expected.tbl";
  auto output_file = ExecuteAndCapture(*query);

  auto expected = GetFileContents(expected_file);
  auto output = GetFileContents(output_file);

  if (!params.asc) {
    std::reverse(expected.begin(), expected.end());
  }

  EXPECT_EQ(output, expected);
}

INSTANTIATE_TEST_SUITE_P(ASMBackend_Asc, OrderByTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .asc = true,
                         }));

INSTANTIATE_TEST_SUITE_P(ASMBackend_Desc, OrderByTest,
                         testing::Values(ParameterValues{
                             .backend = "asm",
                             .asc = false,
                         }));

INSTANTIATE_TEST_SUITE_P(LLVMBackend_Asc, OrderByTest,
                         testing::Values(ParameterValues{
                             .backend = "llvm",
                             .asc = true,
                         }));

INSTANTIATE_TEST_SUITE_P(LLVMBackend_Desc, OrderByTest,
                         testing::Values(ParameterValues{
                             .backend = "llvm",
                             .asc = false,
                         }));