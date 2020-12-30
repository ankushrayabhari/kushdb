#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include "absl/time/civil_time.h"
#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "compile/cpp/cpp_translator.h"
#include "plan/cross_product_operator.h"
#include "plan/expression/aggregate_expression.h"
#include "plan/expression/arithmetic_expression.h"
#include "plan/expression/column_ref_expression.h"
#include "plan/expression/comparison_expression.h"
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
#include "tpch/queries/builder.h"
#include "tpch/schema.h"
#include "util/vector_util.h"

using namespace kush;
using namespace kush::plan;
using namespace kush::compile::cpp;
using namespace kush::catalog;

const Database db = Schema();

// Scan(part)
std::unique_ptr<Operator> ScanPart() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["part"],
                             {"p_partkey", "p_brand", "p_container", "p_size"});
  return std::make_unique<ScanOperator>(std::move(schema), "part");
}

// Select(part)
std::unique_ptr<Operator> SelectPart() {
  auto part = ScanPart();

  std::unique_ptr<Expression> or1;
  {
    std::unique_ptr<Expression> p1 = Geq(ColRef(part, "p_size"), Literal(1));
    std::unique_ptr<Expression> p2 = Leq(ColRef(part, "p_size"), Literal(5));
    std::unique_ptr<Expression> p3 =
        Eq(ColRef(part, "p_brand"), Literal("Brand#22"));
    std::unique_ptr<Expression> p4 =
        Eq(ColRef(part, "p_container"), Literal("SM CASE"));
    std::unique_ptr<Expression> p5 =
        Eq(ColRef(part, "p_container"), Literal("SM BOX"));
    std::unique_ptr<Expression> p6 =
        Eq(ColRef(part, "p_container"), Literal("SM PACK"));
    std::unique_ptr<Expression> p7 =
        Eq(ColRef(part, "p_container"), Literal("SM PKG"));
    or1 = And(
        util::MakeVector(std::move(p1), std::move(p2), std::move(p3),
                         Or(util::MakeVector(std::move(p4), std::move(p5),
                                             std::move(p6), std::move(p7)))));
  }

  std::unique_ptr<Expression> or2;
  {
    std::unique_ptr<Expression> p1 = Geq(ColRef(part, "p_size"), Literal(1));
    std::unique_ptr<Expression> p2 = Leq(ColRef(part, "p_size"), Literal(10));
    std::unique_ptr<Expression> p3 =
        Eq(ColRef(part, "p_brand"), Literal("Brand#23"));
    std::unique_ptr<Expression> p4 =
        Eq(ColRef(part, "p_container"), Literal("MED BAG"));
    std::unique_ptr<Expression> p5 =
        Eq(ColRef(part, "p_container"), Literal("MED BOX"));
    std::unique_ptr<Expression> p6 =
        Eq(ColRef(part, "p_container"), Literal("MED PKG"));
    std::unique_ptr<Expression> p7 =
        Eq(ColRef(part, "p_container"), Literal("MED PACK"));
    or2 = And(
        util::MakeVector(std::move(p1), std::move(p2), std::move(p3),
                         Or(util::MakeVector(std::move(p4), std::move(p5),
                                             std::move(p6), std::move(p7)))));
  }

  std::unique_ptr<Expression> or3;
  {
    std::unique_ptr<Expression> p1 = Geq(ColRef(part, "p_size"), Literal(1));
    std::unique_ptr<Expression> p2 = Leq(ColRef(part, "p_size"), Literal(15));
    std::unique_ptr<Expression> p3 =
        Eq(ColRef(part, "p_brand"), Literal("Brand#12"));
    std::unique_ptr<Expression> p4 =
        Eq(ColRef(part, "p_container"), Literal("LG CASE"));
    std::unique_ptr<Expression> p5 =
        Eq(ColRef(part, "p_container"), Literal("LG BOX"));
    std::unique_ptr<Expression> p6 =
        Eq(ColRef(part, "p_container"), Literal("LG PACK"));
    std::unique_ptr<Expression> p7 =
        Eq(ColRef(part, "p_container"), Literal("LG PKG"));
    or3 = And(
        util::MakeVector(std::move(p1), std::move(p2), std::move(p3),
                         Or(util::MakeVector(std::move(p4), std::move(p5),
                                             std::move(p6), std::move(p7)))));
  }

  auto cond =
      Or(util::MakeVector(std::move(or1), std::move(or2), std::move(or3)));

  OperatorSchema schema;
  schema.AddPassthroughColumns(
      *part, {"p_partkey", "p_brand", "p_container", "p_size"});
  return std::make_unique<SelectOperator>(std::move(schema), std::move(part),
                                          std::move(cond));
}

// Scan(lineitem)
std::unique_ptr<Operator> ScanLinetem() {
  OperatorSchema schema;
  schema.AddGeneratedColumns(db["lineitem"],
                             {"l_extendedprice", "l_discount", "l_quantity",
                              "l_shipmode", "l_shipinstruct", "l_partkey"});
  return std::make_unique<ScanOperator>(std::move(schema), "lineitem");
}

// Select(lineitem)
std::unique_ptr<Operator> SelectLineitem() {
  auto lineitem = ScanLinetem();

  std::unique_ptr<Expression> and1 =
      Eq(ColRef(lineitem, "l_shipinstruct"), Literal("DELIVER IN PERSON"));

  std::unique_ptr<Expression> and2;
  {
    std::unique_ptr<Expression> p1 =
        Eq(ColRef(lineitem, "l_shipmode"), Literal("AIR"));
    std::unique_ptr<Expression> p2 =
        Eq(ColRef(lineitem, "l_shipmode"), Literal("AIR REG"));
    and2 = Or(util::MakeVector(std::move(p1), std::move(p2)));
  }

  std::unique_ptr<Expression> and3;
  {
    std::unique_ptr<Expression> p1 =
        Geq(ColRef(lineitem, "l_quantity"), Literal(8));
    std::unique_ptr<Expression> p2 =
        Leq(ColRef(lineitem, "l_quantity"), Literal(18));
    std::unique_ptr<Expression> p3 =
        Geq(ColRef(lineitem, "l_quantity"), Literal(10));
    std::unique_ptr<Expression> p4 =
        Leq(ColRef(lineitem, "l_quantity"), Literal(20));
    std::unique_ptr<Expression> p5 =
        Geq(ColRef(lineitem, "l_quantity"), Literal(24));
    std::unique_ptr<Expression> p6 =
        Leq(ColRef(lineitem, "l_quantity"), Literal(34));

    std::unique_ptr<Expression> or1 =
        And(util::MakeVector(std::move(p1), std::move(p2)));
    std::unique_ptr<Expression> or2 =
        And(util::MakeVector(std::move(p3), std::move(p4)));
    std::unique_ptr<Expression> or3 =
        And(util::MakeVector(std::move(p5), std::move(p6)));

    and3 = Or(util::MakeVector(std::move(or1), std::move(or2), std::move(or3)));
  }

  auto cond =
      And(util::MakeVector(std::move(and1), std::move(and2), std::move(and3)));

  OperatorSchema schema;
  schema.AddPassthroughColumns(
      *lineitem, {"l_quantity", "l_extendedprice", "l_discount", "l_partkey"});
  return std::make_unique<SelectOperator>(std::move(schema),
                                          std::move(lineitem), std::move(cond));
}

// part JOIN lineitem ON p_partkey = l_partkey
std::unique_ptr<Operator> PartLineitem() {
  auto part = SelectPart();
  auto lineitem = SelectLineitem();

  auto p_partkey = ColRef(part, "p_partkey", 0);
  auto l_partkey = ColRef(lineitem, "l_partkey", 1);

  OperatorSchema schema;
  schema.AddPassthroughColumns(*part, {"p_brand", "p_container", "p_size"}, 0);
  schema.AddPassthroughColumns(
      *lineitem, {"l_quantity", "l_extendedprice", "l_discount"}, 1);
  return std::make_unique<HashJoinOperator>(
      std::move(schema), std::move(part), std::move(lineitem),
      util::MakeVector(std::move(p_partkey)),
      util::MakeVector(std::move(l_partkey)));
}

// select(join)
std::unique_ptr<Operator> SelectPartLineitem() {
  auto base = PartLineitem();

  std::unique_ptr<Expression> or1;
  {
    std::unique_ptr<Expression> p1 = Geq(ColRef(base, "p_size"), Literal(1));
    std::unique_ptr<Expression> p2 = Leq(ColRef(base, "p_size"), Literal(5));
    std::unique_ptr<Expression> p3 =
        Eq(ColRef(base, "p_brand"), Literal("Brand#22"));
    std::unique_ptr<Expression> p4 =
        Eq(ColRef(base, "p_container"), Literal("SM CASE"));
    std::unique_ptr<Expression> p5 =
        Eq(ColRef(base, "p_container"), Literal("SM BOX"));
    std::unique_ptr<Expression> p6 =
        Eq(ColRef(base, "p_container"), Literal("SM PACK"));
    std::unique_ptr<Expression> p7 =
        Eq(ColRef(base, "p_container"), Literal("SM PKG"));
    std::unique_ptr<Expression> p8 =
        Geq(ColRef(base, "l_quantity"), Literal(8));
    std::unique_ptr<Expression> p9 =
        Leq(ColRef(base, "l_quantity"), Literal(18));
    or1 =
        And(util::MakeVector(std::move(p1), std::move(p2), std::move(p3),
                             Or(util::MakeVector(std::move(p4), std::move(p5),
                                                 std::move(p6), std::move(p7))),
                             std::move(p8), std::move(p9)));
  }

  std::unique_ptr<Expression> or2;
  {
    std::unique_ptr<Expression> p1 = Geq(ColRef(base, "p_size"), Literal(1));
    std::unique_ptr<Expression> p2 = Leq(ColRef(base, "p_size"), Literal(10));
    std::unique_ptr<Expression> p3 =
        Eq(ColRef(base, "p_brand"), Literal("Brand#23"));
    std::unique_ptr<Expression> p4 =
        Eq(ColRef(base, "p_container"), Literal("MED BAG"));
    std::unique_ptr<Expression> p5 =
        Eq(ColRef(base, "p_container"), Literal("MED BOX"));
    std::unique_ptr<Expression> p6 =
        Eq(ColRef(base, "p_container"), Literal("MED PKG"));
    std::unique_ptr<Expression> p7 =
        Eq(ColRef(base, "p_container"), Literal("MED PACK"));
    std::unique_ptr<Expression> p8 =
        Geq(ColRef(base, "l_quantity"), Literal(10));
    std::unique_ptr<Expression> p9 =
        Leq(ColRef(base, "l_quantity"), Literal(20));
    or2 =
        And(util::MakeVector(std::move(p1), std::move(p2), std::move(p3),
                             Or(util::MakeVector(std::move(p4), std::move(p5),
                                                 std::move(p6), std::move(p7))),
                             std::move(p8), std::move(p9)));
  }

  std::unique_ptr<Expression> or3;
  {
    std::unique_ptr<Expression> p1 = Geq(ColRef(base, "p_size"), Literal(1));
    std::unique_ptr<Expression> p2 = Leq(ColRef(base, "p_size"), Literal(15));
    std::unique_ptr<Expression> p3 =
        Eq(ColRef(base, "p_brand"), Literal("Brand#12"));
    std::unique_ptr<Expression> p4 =
        Eq(ColRef(base, "p_container"), Literal("LG CASE"));
    std::unique_ptr<Expression> p5 =
        Eq(ColRef(base, "p_container"), Literal("LG BOX"));
    std::unique_ptr<Expression> p6 =
        Eq(ColRef(base, "p_container"), Literal("LG PACK"));
    std::unique_ptr<Expression> p7 =
        Eq(ColRef(base, "p_container"), Literal("LG PKG"));
    std::unique_ptr<Expression> p8 =
        Geq(ColRef(base, "l_quantity"), Literal(24));
    std::unique_ptr<Expression> p9 =
        Leq(ColRef(base, "l_quantity"), Literal(34));
    or3 =
        And(util::MakeVector(std::move(p1), std::move(p2), std::move(p3),
                             Or(util::MakeVector(std::move(p4), std::move(p5),
                                                 std::move(p6), std::move(p7))),
                             std::move(p8), std::move(p9)));
  }
  auto cond =
      Or(util::MakeVector(std::move(or1), std::move(or2), std::move(or3)));

  OperatorSchema schema;
  schema.AddPassthroughColumns(*base, {"l_extendedprice", "l_discount"});
  return std::make_unique<SelectOperator>(std::move(schema), std::move(base),
                                          std::move(cond));
}

// Aggregate
std::unique_ptr<Operator> Agg() {
  auto base = SelectPartLineitem();

  // aggregate
  auto revenue = Sum(Mul(ColRef(base, "l_extendedprice"),
                         Sub(Literal(1), ColRef(base, "l_discount"))));

  // output
  OperatorSchema schema;
  schema.AddDerivedColumn("revenue", VirtColRef(revenue, 0));

  return std::make_unique<GroupByAggregateOperator>(
      std::move(schema), std::move(base),
      std::vector<std::unique_ptr<Expression>>(),
      util::MakeVector(std::move(revenue)));
}

int main() {
  std::unique_ptr<Operator> query = std::make_unique<OutputOperator>(Agg());

  CppTranslator translator(db, *query);
  translator.Translate();
  auto& prog = translator.Program();
  prog.Compile();
  prog.Execute();
  return 0;
}
