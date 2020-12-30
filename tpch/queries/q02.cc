#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include "absl/time/civil_time.h"
#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "compile/cpp/cpp_translator.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/expression/arithmetic_expression.h"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/comparison_expression.h"
#include "plan/expression/literal_expression.h"
#include "plan/expression/string_comparison_expression.h"
#include "plan/expression/virtual_column_ref_expression.h"
#include "plan/group_by_aggregate_operator.h"
#include "plan/hash_join_operator.h"
#include "plan/operator.h"
#include "plan/operator_schema.h"
#include "plan/order_by_operator.h"
#include "plan/output_operator.h"
#include "plan/scan_operator.h"
#include "plan/select_operator.h"
#include "tpch/schema.h"
#include "util/vector_util.h"

using namespace kush;
using namespace kush::plan;
using namespace kush::compile::cpp;
using namespace kush::catalog;

const Database db = Schema();

// Scan(region)
std::unique_ptr<Operator> ScanRegion() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["region"], {"r_regionkey", "r_name"});
  return std::make_unique<ScanOperator>(std::move(schema), "region");
}

// Select(r_name = 'MIDDLE EAST')
std::unique_ptr<Operator> SelectRegion() {
  auto region = ScanRegion();

  auto r_name = std::make_unique<ColumnRefExpression>(
      SqlType::TEXT, 0, region->Schema().GetColumnIndex("r_name"));
  auto literal = std::make_unique<LiteralExpression>("MIDDLE EAST");
  auto eq = std::make_unique<ComparisonExpression>(
      ComparisonType::EQ, std::move(r_name), std::move(literal));

  OperatorSchema schema;
  schema.AddGeneratedColumns(db["region"], {"r_regionkey"});
  return std::make_unique<SelectOperator>(std::move(schema), std::move(region),
                                          std::move(eq));
}

// Scan(nation)
std::unique_ptr<Operator> ScanNation() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["nation"],
                             {"n_name", "n_nationkey", "n_regionkey"});
  return std::make_unique<ScanOperator>(std::move(schema), "nation");
}

// region JOIN nation ON r_regionkey = n_regionkey
std::unique_ptr<Operator> RegionNation() {
  auto region = SelectRegion();
  auto nation = ScanNation();

  auto r_regionkey = std::make_unique<ColumnRefExpression>(
      SqlType::INT, 0, region->Schema().GetColumnIndex("r_regionkey"));
  auto n_regionkey = std::make_unique<ColumnRefExpression>(
      SqlType::INT, 1, region->Schema().GetColumnIndex("n_regionkey"));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*nation, {"n_name", "n_nationkey"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(region), std::move(nation),
      std::move(r_regionkey), std::move(n_regionkey));
}

// Scan(supplier)
std::unique_ptr<Operator> ScanSupplier() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["supplier"],
                             {"s_acctbal", "s_name", "s_address", "s_phone",
                              "s_comment", "s_suppkey", "s_nationkey"});
  return std::make_unique<ScanOperator>(std::move(schema), "supplier");
}

// region_nation JOIN supplier ON n_nationkey = s_nationkey
std::unique_ptr<Operator> RegionNationSupplier() {
  auto region_nation = RegionNation();
  auto supplier = ScanSupplier();

  auto n_nationkey = std::make_unique<ColumnRefExpression>(
      SqlType::INT, 0, region_nation->Schema().GetColumnIndex("n_nationkey"));
  auto s_nationkey = std::make_unique<ColumnRefExpression>(
      SqlType::INT, 1, supplier->Schema().GetColumnIndex("s_nationkey"));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*region_nation, {"n_name"}, 0);
  schema.AddPassthroughColumns(
      *supplier,
      {"s_acctbal", "s_name", "s_address", "s_phone", "s_comment", "s_suppkey"},
      1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(region_nation), std::move(supplier),
      std::move(n_nationkey), std::move(s_nationkey));
}

// Scan(part)
std::unique_ptr<Operator> ScanPart() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["part"],
                             {"p_partkey", "p_mfgr", "p_size", "p_type"});
  return std::make_unique<ScanOperator>(std::move(schema), "part");
}

// Select(p_size = 15 and p_type ENDS WITH 'TIN')
std::unique_ptr<Operator> SelectPart() {
  auto part = ScanPart();

  auto p_size = std::make_unique<ColumnRefExpression>(
      SqlType::INT, 0, part->Schema().GetColumnIndex("p_size"));
  auto literal1 = std::make_unique<LiteralExpression>(15);
  auto eq1 = std::make_unique<ComparisonExpression>(
      ComparisonType::EQ, std::move(p_size), std::move(literal1));

  auto p_type = std::make_unique<ColumnRefExpression>(
      SqlType::TEXT, 0, part->Schema().GetColumnIndex("p_type"));
  auto literal2 = std::make_unique<LiteralExpression>("TIN");
  auto eq2 = std::make_unique<StringComparisonExpression>(
      StringComparisonType::ENDS_WITH, std::move(p_type), std::move(literal2));

  auto conjunction = std::make_unique

      OperatorSchema schema;
  schema.AddGeneratedColumns(db["region"], {"r_regionkey"});
  return std::make_unique<SelectOperator>(std::move(schema), std::move(region),
                                          std::move(eq));
}

// Scan(nation)
std::unique_ptr<Operator> ScanNation() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["nation"],
                             {"n_name", "n_nationkey", "n_regionkey"});
  return std::make_unique<ScanOperator>(std::move(schema), "nation");
}

// region JOIN nation ON r_regionkey = n_regionkey
std::unique_ptr<Operator> RegionNation() {
  auto region = SelectRegion();
  auto nation = ScanNation();

  auto r_regionkey = std::make_unique<ColumnRefExpression>(
      SqlType::INT, 0, region->Schema().GetColumnIndex("r_regionkey"));
  auto n_regionkey = std::make_unique<ColumnRefExpression>(
      SqlType::INT, 1, region->Schema().GetColumnIndex("n_regionkey"));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*nation, {"n_name", "n_nationkey"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(region), std::move(nation),
      std::move(r_regionkey), std::move(n_regionkey));
}

int main() {
  auto query = std::make_unique<OutputOperator>(OrderBy());

  CppTranslator translator(db, *query);
  translator.Translate();
  auto& prog = translator.Program();
  prog.Compile();
  prog.Execute();
  return 0;
}
