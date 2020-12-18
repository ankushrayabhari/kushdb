#pragma once

#include "catalog/catalog.h"
#include "catalog/sql_type.h"

kush::catalog::Database Schema() {
  using namespace kush::catalog;
  Database db;

  {
    auto& table = db.Insert("supplier");
    table.Insert("s_suppkey", SqlType::INT, "tpch/data/s_suppkey.kdb");
    table.Insert("s_name", SqlType::TEXT, "tpch/data/s_name.kdb");
    table.Insert("s_address", SqlType::TEXT, "tpch/data/s_address.kdb");
    table.Insert("s_nationkey", SqlType::INT, "tpch/data/s_nationkey.kdb");
    table.Insert("s_phone", SqlType::TEXT, "tpch/data/s_phone.kdb");
    table.Insert("s_acctbal", SqlType::REAL, "tpch/data/s_acctbal.kdb");
    table.Insert("s_comment", SqlType::TEXT, "tpch/data/s_comment.kdb");
  }

  {
    auto& table = db.Insert("part");
    table.Insert("p_partkey", SqlType::INT, "tpch/data/p_partkey.kdb");
    table.Insert("p_name", SqlType::TEXT, "tpch/data/p_name.kdb");
    table.Insert("p_mfgr", SqlType::TEXT, "tpch/data/p_mfgr.kdb");
    table.Insert("p_brand", SqlType::TEXT, "tpch/data/p_brand.kdb");
    table.Insert("p_type", SqlType::TEXT, "tpch/data/p_type.kdb");
    table.Insert("p_size", SqlType::INT, "tpch/data/p_size.kdb");
    table.Insert("p_container", SqlType::TEXT, "tpch/data/p_container.kdb");
    table.Insert("p_retailprice", SqlType::REAL, "tpch/data/p_retailprice.kdb");
    table.Insert("p_comment", SqlType::TEXT, "tpch/data/p_comment.kdb");
  }

  {
    auto& table = db.Insert("partsupp");
    table.Insert("ps_partkey", SqlType::INT, "tpch/data/ps_partkey.kdb");
    table.Insert("ps_suppkey", SqlType::INT, "tpch/data/ps_suppkey.kdb");
    table.Insert("ps_availqty", SqlType::INT, "tpch/data/ps_availqty.kdb");
    table.Insert("ps_supplycost", SqlType::REAL, "tpch/data/ps_supplycost.kdb");
    table.Insert("ps_comment", SqlType::TEXT, "tpch/data/ps_comment.kdb");
  }

  {
    auto& table = db.Insert("customer");
    table.Insert("c_custkey", SqlType::INT, "tpch/data/c_custkey.kdb");
    table.Insert("c_name", SqlType::TEXT, "tpch/data/c_name.kdb");
    table.Insert("c_address", SqlType::TEXT, "tpch/data/c_address.kdb");
    table.Insert("c_nationkey", SqlType::INT, "tpch/data/c_nationkey.kdb");
    table.Insert("c_phone", SqlType::TEXT, "tpch/data/c_phone.kdb");
    table.Insert("c_acctbal", SqlType::REAL, "tpch/data/c_acctbal.kdb");
    table.Insert("c_mktsegment", SqlType::TEXT, "tpch/data/c_mktsegment.kdb");
    table.Insert("c_comment", SqlType::TEXT, "tpch/data/c_comment.kdb");
  }

  {
    auto& table = db.Insert("orders");
    table.Insert("o_orderkey", SqlType::INT, "tpch/data/o_orderkey.kdb");
    table.Insert("o_custkey", SqlType::INT, "tpch/data/o_custkey.kdb");
    table.Insert("o_orderstatus", SqlType::TEXT, "tpch/data/o_orderstatus.kdb");
    table.Insert("o_totalprice", SqlType::REAL, "tpch/data/o_totalprice.kdb");
    table.Insert("o_orderdate", SqlType::DATE, "tpch/data/o_orderdate.kdb");
    table.Insert("o_orderpriority", SqlType::TEXT,
                 "tpch/data/o_orderpriority.kdb");
    table.Insert("o_clerk", SqlType::TEXT, "tpch/data/o_clerk.kdb");
    table.Insert("o_shippriority", SqlType::INT,
                 "tpch/data/o_shippriority.kdb");
    table.Insert("o_comment", SqlType::TEXT, "tpch/data/o_comment.kdb");
  }

  {
    auto& table = db.Insert("lineitem");
    table.Insert("l_orderkey", SqlType::INT, "tpch/data/l_orderkey.kdb");
    table.Insert("l_partkey", SqlType::INT, "tpch/data/l_partkey.kdb");
    table.Insert("l_suppkey", SqlType::INT, "tpch/data/l_suppkey.kdb");
    table.Insert("l_linenumber", SqlType::INT, "tpch/data/l_linenumber.kdb");
    table.Insert("l_quantity", SqlType::REAL, "tpch/data/l_quantity.kdb");
    table.Insert("l_extendedprice", SqlType::REAL,
                 "tpch/data/l_extendedprice.kdb");
    table.Insert("l_discount", SqlType::REAL, "tpch/data/l_discount.kdb");
    table.Insert("l_tax", SqlType::REAL, "tpch/data/l_tax.kdb");
    table.Insert("l_returnflag", SqlType::TEXT, "tpch/data/l_returnflag.kdb");

    table.Insert("l_linestatus", SqlType::TEXT, "tpch/data/l_linestatus.kdb");
    table.Insert("l_shipdate", SqlType::DATE, "tpch/data/l_shipdate.kdb");
    table.Insert("l_commitdate", SqlType::DATE, "tpch/data/l_commitdate.kdb");
    table.Insert("l_receiptdate", SqlType::DATE, "tpch/data/l_receiptdate.kdb");
    table.Insert("l_shipinstruct", SqlType::TEXT,
                 "tpch/data/l_shipinstruct.kdb");
    table.Insert("l_shipmode", SqlType::TEXT, "tpch/data/l_shipmode.kdb");
    table.Insert("l_comment", SqlType::TEXT, "tpch/data/l_comment.kdb");
  }

  {
    auto& table = db.Insert("nation");
    table.Insert("n_nationkey", SqlType::INT, "tpch/data/n_nationkey.kdb");
    table.Insert("n_name", SqlType::TEXT, "tpch/data/n_name.kdb");
    table.Insert("n_regionkey", SqlType::INT, "tpch/data/n_regionkey.kdb");
    table.Insert("n_comment", SqlType::TEXT, "tpch/data/n_comment.kdb");
  }

  {
    auto& table = db.Insert("region");
    table.Insert("r_regionkey", SqlType::INT, "tpch/data/r_regionkey.kdb");
    table.Insert("r_name", SqlType::TEXT, "tpch/data/r_name.kdb");
    table.Insert("r_comment", SqlType::TEXT, "tpch/data/r_comment.kdb");
  }

  return db;
}