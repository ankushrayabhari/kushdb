#pragma once

#include "catalog/catalog.h"
#include "catalog/sql_type.h"

kush::catalog::Database Schema() {
  using namespace kush::catalog;
  Database db;

  {
    auto& table = db.Insert("supplier");
    table.Insert("s_suppkey", SqlType::INT,
                 "benchmark/jcch10/data/s_suppkey.kdb", "",
                 "benchmark/jcch10/data/s_suppkey.kdbindex");
    table.Insert("s_name", SqlType::TEXT, "benchmark/jcch10/data/s_name.kdb",
                 "", "benchmark/jcch10/data/s_name.kdbindex");
    table.Insert("s_address", SqlType::TEXT,
                 "benchmark/jcch10/data/s_address.kdb", "",
                 "benchmark/jcch10/data/s_address.kdbindex");
    table.Insert("s_nationkey", SqlType::INT,
                 "benchmark/jcch10/data/s_nationkey.kdb", "",
                 "benchmark/jcch10/data/s_nationkey.kdbindex");
    table.Insert("s_phone", SqlType::TEXT, "benchmark/jcch10/data/s_phone.kdb",
                 "", "benchmark/jcch10/data/s_phone.kdbindex");
    table.Insert("s_acctbal", SqlType::REAL,
                 "benchmark/jcch10/data/s_acctbal.kdb", "",
                 "benchmark/jcch10/data/s_acctbal.kdbindex");
    table.Insert("s_comment", SqlType::TEXT,
                 "benchmark/jcch10/data/s_comment.kdb", "",
                 "benchmark/jcch10/data/s_comment.kdbindex");
  }

  {
    auto& table = db.Insert("part");
    table.Insert("p_partkey", SqlType::INT,
                 "benchmark/jcch10/data/p_partkey.kdb", "",
                 "benchmark/jcch10/data/p_partkey.kdbindex");
    table.Insert("p_name", SqlType::TEXT, "benchmark/jcch10/data/p_name.kdb",
                 "", "benchmark/jcch10/data/p_name.kdbindex");
    table.Insert("p_mfgr", SqlType::TEXT, "benchmark/jcch10/data/p_mfgr.kdb",
                 "", "benchmark/jcch10/data/p_mfgr.kdbindex");
    table.Insert("p_brand", SqlType::TEXT, "benchmark/jcch10/data/p_brand.kdb",
                 "", "benchmark/jcch10/data/p_brand.kdbindex");
    table.Insert("p_type", SqlType::TEXT, "benchmark/jcch10/data/p_type.kdb",
                 "", "benchmark/jcch10/data/p_type.kdbindex");
    table.Insert("p_size", SqlType::INT, "benchmark/jcch10/data/p_size.kdb", "",
                 "benchmark/jcch10/data/p_size.kdbindex");
    table.Insert("p_container", SqlType::TEXT,
                 "benchmark/jcch10/data/p_container.kdb", "",
                 "benchmark/jcch10/data/p_container.kdbindex");
    table.Insert("p_retailprice", SqlType::REAL,
                 "benchmark/jcch10/data/p_retailprice.kdb", "",
                 "benchmark/jcch10/data/p_retailprice.kdbindex");
    table.Insert("p_comment", SqlType::TEXT,
                 "benchmark/jcch10/data/p_comment.kdb", "",
                 "benchmark/jcch10/data/p_comment.kdbindex");
  }

  {
    auto& table = db.Insert("partsupp");
    table.Insert("ps_partkey", SqlType::INT,
                 "benchmark/jcch10/data/ps_partkey.kdb", "",
                 "benchmark/jcch10/data/ps_partkey.kdbindex");
    table.Insert("ps_suppkey", SqlType::INT,
                 "benchmark/jcch10/data/ps_suppkey.kdb", "",
                 "benchmark/jcch10/data/ps_suppkey.kdbindex");
    table.Insert("ps_availqty", SqlType::INT,
                 "benchmark/jcch10/data/ps_availqty.kdb", "",
                 "benchmark/jcch10/data/ps_availqty.kdbindex");
    table.Insert("ps_supplycost", SqlType::REAL,
                 "benchmark/jcch10/data/ps_supplycost.kdb", "",
                 "benchmark/jcch10/data/ps_supplycost.kdbindex");
    table.Insert("ps_comment", SqlType::TEXT,
                 "benchmark/jcch10/data/ps_comment.kdb", "",
                 "benchmark/jcch10/data/ps_comment.kdbindex");
  }

  {
    auto& table = db.Insert("customer");
    table.Insert("c_custkey", SqlType::INT,
                 "benchmark/jcch10/data/c_custkey.kdb", "",
                 "benchmark/jcch10/data/c_custkey.kdbindex");
    table.Insert("c_name", SqlType::TEXT, "benchmark/jcch10/data/c_name.kdb",
                 "", "benchmark/jcch10/data/c_name.kdbindex");
    table.Insert("c_address", SqlType::TEXT,
                 "benchmark/jcch10/data/c_address.kdb", "",
                 "benchmark/jcch10/data/c_address.kdbindex");
    table.Insert("c_nationkey", SqlType::INT,
                 "benchmark/jcch10/data/c_nationkey.kdb", "",
                 "benchmark/jcch10/data/c_nationkey.kdbindex");
    table.Insert("c_phone", SqlType::TEXT, "benchmark/jcch10/data/c_phone.kdb",
                 "", "benchmark/jcch10/data/c_phone.kdbindex");
    table.Insert("c_acctbal", SqlType::REAL,
                 "benchmark/jcch10/data/c_acctbal.kdb", "",
                 "benchmark/jcch10/data/c_acctbal.kdbindex");
    table.Insert("c_mktsegment", SqlType::TEXT,
                 "benchmark/jcch10/data/c_mktsegment.kdb", "",
                 "benchmark/jcch10/data/c_mktsegment.kdbindex");
    table.Insert("c_comment", SqlType::TEXT,
                 "benchmark/jcch10/data/c_comment.kdb", "",
                 "benchmark/jcch10/data/c_comment.kdbindex");
  }

  {
    auto& table = db.Insert("orders");
    table.Insert("o_orderkey", SqlType::INT,
                 "benchmark/jcch10/data/o_orderkey.kdb", "",
                 "benchmark/jcch10/data/o_orderkey.kdbindex");
    table.Insert("o_custkey", SqlType::INT,
                 "benchmark/jcch10/data/o_custkey.kdb", "",
                 "benchmark/jcch10/data/o_custkey.kdbindex");
    table.Insert("o_orderstatus", SqlType::TEXT,
                 "benchmark/jcch10/data/o_orderstatus.kdb", "",
                 "benchmark/jcch10/data/o_orderstatus.kdbindex");
    table.Insert("o_totalprice", SqlType::REAL,
                 "benchmark/jcch10/data/o_totalprice.kdb", "",
                 "benchmark/jcch10/data/o_totalprice.kdbindex");
    table.Insert("o_orderdate", SqlType::DATE,
                 "benchmark/jcch10/data/o_orderdate.kdb", "",
                 "benchmark/jcch10/data/o_orderdate.kdbindex");
    table.Insert("o_orderpriority", SqlType::TEXT,
                 "benchmark/jcch10/data/o_orderpriority.kdb", "",
                 "benchmark/jcch10/data/o_orderpriority.kdbindex");
    table.Insert("o_clerk", SqlType::TEXT, "benchmark/jcch10/data/o_clerk.kdb",
                 "", "benchmark/jcch10/data/o_clerk.kdbindex");
    table.Insert("o_shippriority", SqlType::INT,
                 "benchmark/jcch10/data/o_shippriority.kdb", "",
                 "benchmark/jcch10/data/o_shippriority.kdbindex");
    table.Insert("o_comment", SqlType::TEXT,
                 "benchmark/jcch10/data/o_comment.kdb", "",
                 "benchmark/jcch10/data/o_comment.kdbindex");
  }

  {
    auto& table = db.Insert("lineitem");
    table.Insert("l_orderkey", SqlType::INT,
                 "benchmark/jcch10/data/l_orderkey.kdb", "",
                 "benchmark/jcch10/data/l_orderkey.kdbindex");
    table.Insert("l_partkey", SqlType::INT,
                 "benchmark/jcch10/data/l_partkey.kdb", "",
                 "benchmark/jcch10/data/l_partkey.kdbindex");
    table.Insert("l_suppkey", SqlType::INT,
                 "benchmark/jcch10/data/l_suppkey.kdb", "",
                 "benchmark/jcch10/data/l_suppkey.kdbindex");
    table.Insert("l_linenumber", SqlType::INT,
                 "benchmark/jcch10/data/l_linenumber.kdb", "",
                 "benchmark/jcch10/data/l_linenumber.kdbindex");
    table.Insert("l_quantity", SqlType::REAL,
                 "benchmark/jcch10/data/l_quantity.kdb", "",
                 "benchmark/jcch10/data/l_quantity.kdbindex");
    table.Insert("l_extendedprice", SqlType::REAL,
                 "benchmark/jcch10/data/l_extendedprice.kdb", "",
                 "benchmark/jcch10/data/l_extendedprice.kdbindex");
    table.Insert("l_discount", SqlType::REAL,
                 "benchmark/jcch10/data/l_discount.kdb", "",
                 "benchmark/jcch10/data/l_discount.kdbindex");
    table.Insert("l_tax", SqlType::REAL, "benchmark/jcch10/data/l_tax.kdb", "",
                 "benchmark/jcch10/data/l_tax.kdbindex");
    table.Insert("l_returnflag", SqlType::TEXT,
                 "benchmark/jcch10/data/l_returnflag.kdb", "",
                 "benchmark/jcch10/data/l_returnflag.kdbindex");
    table.Insert("l_linestatus", SqlType::TEXT,
                 "benchmark/jcch10/data/l_linestatus.kdb", "",
                 "benchmark/jcch10/data/l_linestatus.kdbindex");
    table.Insert("l_shipdate", SqlType::DATE,
                 "benchmark/jcch10/data/l_shipdate.kdb", "",
                 "benchmark/jcch10/data/l_shipdate.kdbindex");
    table.Insert("l_commitdate", SqlType::DATE,
                 "benchmark/jcch10/data/l_commitdate.kdb", "",
                 "benchmark/jcch10/data/l_commitdate.kdbindex");
    table.Insert("l_receiptdate", SqlType::DATE,
                 "benchmark/jcch10/data/l_receiptdate.kdb", "",
                 "benchmark/jcch10/data/l_receiptdate.kdbindex");
    table.Insert("l_shipinstruct", SqlType::TEXT,
                 "benchmark/jcch10/data/l_shipinstruct.kdb", "",
                 "benchmark/jcch10/data/l_shipinstruct.kdbindex");
    table.Insert("l_shipmode", SqlType::TEXT,
                 "benchmark/jcch10/data/l_shipmode.kdb", "",
                 "benchmark/jcch10/data/l_shipmode.kdbindex");
    table.Insert("l_comment", SqlType::TEXT,
                 "benchmark/jcch10/data/l_comment.kdb", "",
                 "benchmark/jcch10/data/l_comment.kdbindex");
  }

  {
    auto& table = db.Insert("nation");
    table.Insert("n_nationkey", SqlType::INT,
                 "benchmark/jcch10/data/n_nationkey.kdb", "",
                 "benchmark/jcch10/data/n_nationkey.kdbindex");
    table.Insert("n_name", SqlType::TEXT, "benchmark/jcch10/data/n_name.kdb",
                 "", "benchmark/jcch10/data/n_name.kdbindex");
    table.Insert("n_regionkey", SqlType::INT,
                 "benchmark/jcch10/data/n_regionkey.kdb", "",
                 "benchmark/jcch10/data/n_regionkey.kdbindex");
    table.Insert("n_comment", SqlType::TEXT,
                 "benchmark/jcch10/data/n_comment.kdb", "",
                 "benchmark/jcch10/data/n_comment.kdbindex");
  }

  {
    auto& table = db.Insert("region");
    table.Insert("r_regionkey", SqlType::INT,
                 "benchmark/jcch10/data/r_regionkey.kdb", "",
                 "benchmark/jcch10/data/r_regionkey.kdbindex");
    table.Insert("r_name", SqlType::TEXT, "benchmark/jcch10/data/r_name.kdb",
                 "", "benchmark/jcch10/data/r_name.kdbindex");
    table.Insert("r_comment", SqlType::TEXT,
                 "benchmark/jcch10/data/r_comment.kdb", "",
                 "benchmark/jcch10/data/r_comment.kdbindex");
  }

  return db;
}