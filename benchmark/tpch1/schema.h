#pragma once

#include "catalog/catalog.h"
#include "catalog/sql_type.h"

kush::catalog::Database Schema() {
  using namespace kush::catalog;
  Database db;

  {
    auto& table = db.Insert("supplier");
    table.Insert("s_suppkey", Type::Int(), "benchmark/tpch1/data/s_suppkey.kdb",
                 "", "benchmark/tpch1/data/s_suppkey.kdbindex");
    table.Insert("s_name", Type::Text(), "benchmark/tpch1/data/s_name.kdb", "",
                 "benchmark/tpch1/data/s_name.kdbindex");
    table.Insert("s_address", Type::Text(),
                 "benchmark/tpch1/data/s_address.kdb", "",
                 "benchmark/tpch1/data/s_address.kdbindex");
    table.Insert("s_nationkey", Type::Int(),
                 "benchmark/tpch1/data/s_nationkey.kdb", "",
                 "benchmark/tpch1/data/s_nationkey.kdbindex");
    table.Insert("s_phone", Type::Text(), "benchmark/tpch1/data/s_phone.kdb",
                 "", "benchmark/tpch1/data/s_phone.kdbindex");
    table.Insert("s_acctbal", Type::Real(),
                 "benchmark/tpch1/data/s_acctbal.kdb", "",
                 "benchmark/tpch1/data/s_acctbal.kdbindex");
    table.Insert("s_comment", Type::Text(),
                 "benchmark/tpch1/data/s_comment.kdb", "",
                 "benchmark/tpch1/data/s_comment.kdbindex");
  }

  {
    auto& table = db.Insert("part");
    table.Insert("p_partkey", Type::Int(), "benchmark/tpch1/data/p_partkey.kdb",
                 "", "benchmark/tpch1/data/p_partkey.kdbindex");
    table.Insert("p_name", Type::Text(), "benchmark/tpch1/data/p_name.kdb", "",
                 "benchmark/tpch1/data/p_name.kdbindex");
    table.Insert("p_mfgr", Type::Text(), "benchmark/tpch1/data/p_mfgr.kdb", "",
                 "benchmark/tpch1/data/p_mfgr.kdbindex");
    table.Insert("p_brand", Type::Text(), "benchmark/tpch1/data/p_brand.kdb",
                 "", "benchmark/tpch1/data/p_brand.kdbindex");
    table.Insert("p_type", Type::Text(), "benchmark/tpch1/data/p_type.kdb", "",
                 "benchmark/tpch1/data/p_type.kdbindex");
    table.Insert("p_size", Type::Int(), "benchmark/tpch1/data/p_size.kdb", "",
                 "benchmark/tpch1/data/p_size.kdbindex");
    table.Insert("p_container", Type::Text(),
                 "benchmark/tpch1/data/p_container.kdb", "",
                 "benchmark/tpch1/data/p_container.kdbindex");
    table.Insert("p_retailprice", Type::Real(),
                 "benchmark/tpch1/data/p_retailprice.kdb", "",
                 "benchmark/tpch1/data/p_retailprice.kdbindex");
    table.Insert("p_comment", Type::Text(),
                 "benchmark/tpch1/data/p_comment.kdb", "",
                 "benchmark/tpch1/data/p_comment.kdbindex");
  }

  {
    auto& table = db.Insert("partsupp");
    table.Insert("ps_partkey", Type::Int(),
                 "benchmark/tpch1/data/ps_partkey.kdb", "",
                 "benchmark/tpch1/data/ps_partkey.kdbindex");
    table.Insert("ps_suppkey", Type::Int(),
                 "benchmark/tpch1/data/ps_suppkey.kdb", "",
                 "benchmark/tpch1/data/ps_suppkey.kdbindex");
    table.Insert("ps_availqty", Type::Int(),
                 "benchmark/tpch1/data/ps_availqty.kdb", "",
                 "benchmark/tpch1/data/ps_availqty.kdbindex");
    table.Insert("ps_supplycost", Type::Real(),
                 "benchmark/tpch1/data/ps_supplycost.kdb", "",
                 "benchmark/tpch1/data/ps_supplycost.kdbindex");
    table.Insert("ps_comment", Type::Text(),
                 "benchmark/tpch1/data/ps_comment.kdb", "",
                 "benchmark/tpch1/data/ps_comment.kdbindex");
  }

  {
    auto& table = db.Insert("customer");
    table.Insert("c_custkey", Type::Int(), "benchmark/tpch1/data/c_custkey.kdb",
                 "", "benchmark/tpch1/data/c_custkey.kdbindex");
    table.Insert("c_name", Type::Text(), "benchmark/tpch1/data/c_name.kdb", "",
                 "benchmark/tpch1/data/c_name.kdbindex");
    table.Insert("c_address", Type::Text(),
                 "benchmark/tpch1/data/c_address.kdb", "",
                 "benchmark/tpch1/data/c_address.kdbindex");
    table.Insert("c_nationkey", Type::Int(),
                 "benchmark/tpch1/data/c_nationkey.kdb", "",
                 "benchmark/tpch1/data/c_nationkey.kdbindex");
    table.Insert("c_phone", Type::Text(), "benchmark/tpch1/data/c_phone.kdb",
                 "", "benchmark/tpch1/data/c_phone.kdbindex");
    table.Insert("c_acctbal", Type::Real(),
                 "benchmark/tpch1/data/c_acctbal.kdb", "",
                 "benchmark/tpch1/data/c_acctbal.kdbindex");
    table.Insert("c_mktsegment", Type::Text(),
                 "benchmark/tpch1/data/c_mktsegment.kdb", "",
                 "benchmark/tpch1/data/c_mktsegment.kdbindex");
    table.Insert("c_comment", Type::Text(),
                 "benchmark/tpch1/data/c_comment.kdb", "",
                 "benchmark/tpch1/data/c_comment.kdbindex");
  }

  {
    auto& table = db.Insert("orders");
    table.Insert("o_orderkey", Type::Int(),
                 "benchmark/tpch1/data/o_orderkey.kdb", "",
                 "benchmark/tpch1/data/o_orderkey.kdbindex");
    table.Insert("o_custkey", Type::Int(), "benchmark/tpch1/data/o_custkey.kdb",
                 "", "benchmark/tpch1/data/o_custkey.kdbindex");
    table.Insert("o_orderstatus", Type::Text(),
                 "benchmark/tpch1/data/o_orderstatus.kdb", "",
                 "benchmark/tpch1/data/o_orderstatus.kdbindex");
    table.Insert("o_totalprice", Type::Real(),
                 "benchmark/tpch1/data/o_totalprice.kdb", "",
                 "benchmark/tpch1/data/o_totalprice.kdbindex");
    table.Insert("o_orderdate", Type::Date(),
                 "benchmark/tpch1/data/o_orderdate.kdb", "",
                 "benchmark/tpch1/data/o_orderdate.kdbindex");
    table.Insert("o_orderpriority", Type::Text(),
                 "benchmark/tpch1/data/o_orderpriority.kdb", "",
                 "benchmark/tpch1/data/o_orderpriority.kdbindex");
    table.Insert("o_clerk", Type::Text(), "benchmark/tpch1/data/o_clerk.kdb",
                 "", "benchmark/tpch1/data/o_clerk.kdbindex");
    table.Insert("o_shippriority", Type::Int(),
                 "benchmark/tpch1/data/o_shippriority.kdb", "",
                 "benchmark/tpch1/data/o_shippriority.kdbindex");
    table.Insert("o_comment", Type::Text(),
                 "benchmark/tpch1/data/o_comment.kdb", "",
                 "benchmark/tpch1/data/o_comment.kdbindex");
  }

  {
    auto& table = db.Insert("lineitem");
    table.Insert("l_orderkey", Type::Int(),
                 "benchmark/tpch1/data/l_orderkey.kdb", "",
                 "benchmark/tpch1/data/l_orderkey.kdbindex");
    table.Insert("l_partkey", Type::Int(), "benchmark/tpch1/data/l_partkey.kdb",
                 "", "benchmark/tpch1/data/l_partkey.kdbindex");
    table.Insert("l_suppkey", Type::Int(), "benchmark/tpch1/data/l_suppkey.kdb",
                 "", "benchmark/tpch1/data/l_suppkey.kdbindex");
    table.Insert("l_linenumber", Type::Int(),
                 "benchmark/tpch1/data/l_linenumber.kdb", "",
                 "benchmark/tpch1/data/l_linenumber.kdbindex");
    table.Insert("l_quantity", Type::Real(),
                 "benchmark/tpch1/data/l_quantity.kdb", "",
                 "benchmark/tpch1/data/l_quantity.kdbindex");
    table.Insert("l_extendedprice", Type::Real(),
                 "benchmark/tpch1/data/l_extendedprice.kdb", "",
                 "benchmark/tpch1/data/l_extendedprice.kdbindex");
    table.Insert("l_discount", Type::Real(),
                 "benchmark/tpch1/data/l_discount.kdb", "",
                 "benchmark/tpch1/data/l_discount.kdbindex");
    table.Insert("l_tax", Type::Real(), "benchmark/tpch1/data/l_tax.kdb", "",
                 "benchmark/tpch1/data/l_tax.kdbindex");
    table.Insert("l_returnflag", Type::Text(),
                 "benchmark/tpch1/data/l_returnflag.kdb", "",
                 "benchmark/tpch1/data/l_returnflag.kdbindex");
    table.Insert("l_linestatus", Type::Text(),
                 "benchmark/tpch1/data/l_linestatus.kdb", "",
                 "benchmark/tpch1/data/l_linestatus.kdbindex");
    table.Insert("l_shipdate", Type::Date(),
                 "benchmark/tpch1/data/l_shipdate.kdb", "",
                 "benchmark/tpch1/data/l_shipdate.kdbindex");
    table.Insert("l_commitdate", Type::Date(),
                 "benchmark/tpch1/data/l_commitdate.kdb", "",
                 "benchmark/tpch1/data/l_commitdate.kdbindex");
    table.Insert("l_receiptdate", Type::Date(),
                 "benchmark/tpch1/data/l_receiptdate.kdb", "",
                 "benchmark/tpch1/data/l_receiptdate.kdbindex");
    table.Insert("l_shipinstruct", Type::Text(),
                 "benchmark/tpch1/data/l_shipinstruct.kdb", "",
                 "benchmark/tpch1/data/l_shipinstruct.kdbindex");
    table.Insert("l_shipmode", Type::Text(),
                 "benchmark/tpch1/data/l_shipmode.kdb", "",
                 "benchmark/tpch1/data/l_shipmode.kdbindex");
    table.Insert("l_comment", Type::Text(),
                 "benchmark/tpch1/data/l_comment.kdb", "",
                 "benchmark/tpch1/data/l_comment.kdbindex");
  }

  {
    auto& table = db.Insert("nation");
    table.Insert("n_nationkey", Type::Int(),
                 "benchmark/tpch1/data/n_nationkey.kdb", "",
                 "benchmark/tpch1/data/n_nationkey.kdbindex");
    table.Insert("n_name", Type::Text(), "benchmark/tpch1/data/n_name.kdb", "",
                 "benchmark/tpch1/data/n_name.kdbindex");
    table.Insert("n_regionkey", Type::Int(),
                 "benchmark/tpch1/data/n_regionkey.kdb", "",
                 "benchmark/tpch1/data/n_regionkey.kdbindex");
    table.Insert("n_comment", Type::Text(),
                 "benchmark/tpch1/data/n_comment.kdb", "",
                 "benchmark/tpch1/data/n_comment.kdbindex");
  }

  {
    auto& table = db.Insert("region");
    table.Insert("r_regionkey", Type::Int(),
                 "benchmark/tpch1/data/r_regionkey.kdb", "",
                 "benchmark/tpch1/data/r_regionkey.kdbindex");
    table.Insert("r_name", Type::Text(), "benchmark/tpch1/data/r_name.kdb", "",
                 "benchmark/tpch1/data/r_name.kdbindex");
    table.Insert("r_comment", Type::Text(),
                 "benchmark/tpch1/data/r_comment.kdb", "",
                 "benchmark/tpch1/data/r_comment.kdbindex");
  }

  return db;
}