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

class HashJoinTest : public testing::TestWithParam<ParameterValues> {};

ABSL_DECLARE_FLAG(std::string, backend);

TEST_P(HashJoinTest, BooleanCol) {
  auto params = GetParam();
  absl::SetFlag(&FLAGS_backend, params.backend);

  auto db = Schema();

  std::unique_ptr<Operator> query;
  {
    std::unique_ptr<Operator> s1;
    {
      OperatorSchema schema;
      schema.AddGeneratedColumns(db["info"], {"cheated"});
      s1 = std::make_unique<ScanOperator>(std::move(schema), db["info"]);
    }

    std::unique_ptr<Operator> s2;
    {
      OperatorSchema schema;
      schema.AddGeneratedColumns(db["info"], {"cheated"});
      s2 = std::make_unique<ScanOperator>(std::move(schema), db["info"]);
    }

    auto col1 = ColRef(s1, "cheated", 0);
    auto col2 = ColRef(s2, "cheated", 1);

    OperatorSchema schema;
    schema.AddPassthroughColumns(*s1, 0);
    schema.AddPassthroughColumns(*s2, 1);
    query = std::make_unique<OutputOperator>(std::make_unique<HashJoinOperator>(
        std::move(schema), std::move(s1), std::move(s2),
        util::MakeVector(std::move(col1)), util::MakeVector(std::move(col2))));
  }
  auto expected_file =
      "end_to_end_test/hash_join/boolean_col_join_expected.tbl";
  auto output_file = ExecuteAndCapture(*query);

  auto expected = GetFileContents(expected_file);
  auto output = GetFileContents(output_file);
  std::sort(expected.begin(), expected.end());
  std::sort(output.begin(), output.end());
  EXPECT_EQ(output, expected);
}

INSTANTIATE_TEST_SUITE_P(ASMBackend, HashJoinTest,
                         testing::Values(ParameterValues{.backend = "asm"}));

/*
INSTANTIATE_TEST_SUITE_P(LLVMBackend, HashJoinTest,
                         testing::Values(ParameterValues{.backend = "llvm"}));
*/