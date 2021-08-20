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

TEST_P(HashJoinTest, IntColSelfJoin) {
  auto params = GetParam();
  absl::SetFlag(&FLAGS_backend, params.backend);

  auto db = Schema();

  std::unique_ptr<Operator> query;
  {
    std::unique_ptr<Operator> s1;
    {
      OperatorSchema schema;
      schema.AddGeneratedColumns(db["supplier"], {"s_suppkey"});
      s1 = std::make_unique<ScanOperator>(std::move(schema), db["supplier"]);
    }

    std::unique_ptr<Operator> s2;
    {
      OperatorSchema schema;
      schema.AddGeneratedColumns(db["supplier"], {"s_suppkey"});
      s2 = std::make_unique<ScanOperator>(std::move(schema), db["supplier"]);
    }

    auto col1 = ColRef(s1, "s_suppkey", 0);
    auto col2 = ColRef(s2, "s_suppkey", 1);

    OperatorSchema schema;
    schema.AddPassthroughColumns(*s1, 0);
    schema.AddPassthroughColumns(*s2, 1);
    query = std::make_unique<OutputOperator>(std::make_unique<HashJoinOperator>(
        std::move(schema), std::move(s1), std::move(s2),
        util::MakeVector(std::move(col1)), util::MakeVector(std::move(col2))));
  }

  std::stringstream buffer;
  std::streambuf *sbuf = std::cout.rdbuf();
  std::cout.rdbuf(buffer.rdbuf());

  kush::compile::QueryTranslator translator(*query);
  auto [codegen, prog] = translator.Translate();
  prog->Compile();
  using compute_fn = std::add_pointer<void()>::type;
  auto compute = reinterpret_cast<compute_fn>(prog->GetFunction("compute"));
  compute();

  int c1, c2;
  while (static_cast<bool>(buffer >> c1)) {
    char tmp;
    buffer >> tmp;
    buffer >> c2;
    buffer >> tmp;
    EXPECT_EQ(c1, c2);
  }

  std::cout.rdbuf(sbuf);
}

INSTANTIATE_TEST_SUITE_P(ASMBackend, HashJoinTest,
                         testing::Values(ParameterValues{.backend = "asm"}));

INSTANTIATE_TEST_SUITE_P(LLVMBackend, HashJoinTest,
                         testing::Values(ParameterValues{.backend = "llvm"}));