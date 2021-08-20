#pragma once

#include "catalog/catalog.h"
#include "catalog/sql_type.h"

kush::catalog::Database Schema() {
  using namespace kush::catalog;
  Database db;

  {
    auto& table = db.Insert("supplier");
    table.Insert("s_suppkey", SqlType::INT,
                 "end_to_end_test/data/s_suppkey.kdb");
    table.Insert("s_name", SqlType::TEXT, "end_to_end_test/data/s_name.kdb");
    table.Insert("s_address", SqlType::TEXT,
                 "end_to_end_test/data/s_address.kdb");
    table.Insert("s_nationkey", SqlType::INT,
                 "end_to_end_test/data/s_nationkey.kdb");
    table.Insert("s_phone", SqlType::TEXT, "end_to_end_test/data/s_phone.kdb");
    table.Insert("s_acctbal", SqlType::REAL,
                 "end_to_end_test/data/s_acctbal.kdb");
    table.Insert("s_comment", SqlType::TEXT,
                 "end_to_end_test/data/s_comment.kdb");
  }

  {
    auto& table = db.Insert("part");
    table.Insert("p_partkey", SqlType::INT,
                 "end_to_end_test/data/p_partkey.kdb");
    table.Insert("p_name", SqlType::TEXT, "end_to_end_test/data/p_name.kdb");
    table.Insert("p_mfgr", SqlType::TEXT, "end_to_end_test/data/p_mfgr.kdb");
    table.Insert("p_brand", SqlType::TEXT, "end_to_end_test/data/p_brand.kdb");
    table.Insert("p_type", SqlType::TEXT, "end_to_end_test/data/p_type.kdb");
    table.Insert("p_size", SqlType::INT, "end_to_end_test/data/p_size.kdb");
    table.Insert("p_container", SqlType::TEXT,
                 "end_to_end_test/data/p_container.kdb");
    table.Insert("p_retailprice", SqlType::REAL,
                 "end_to_end_test/data/p_retailprice.kdb");
    table.Insert("p_comment", SqlType::TEXT,
                 "end_to_end_test/data/p_comment.kdb");
  }

  {
    auto& table = db.Insert("partsupp");
    table.Insert("ps_partkey", SqlType::INT,
                 "end_to_end_test/data/ps_partkey.kdb");
    table.Insert("ps_suppkey", SqlType::INT,
                 "end_to_end_test/data/ps_suppkey.kdb");
    table.Insert("ps_availqty", SqlType::INT,
                 "end_to_end_test/data/ps_availqty.kdb");
    table.Insert("ps_supplycost", SqlType::REAL,
                 "end_to_end_test/data/ps_supplycost.kdb");
  }

  {
    auto& table = db.Insert("customer");
    table.Insert("c_custkey", SqlType::INT,
                 "end_to_end_test/data/c_custkey.kdb");
    table.Insert("c_name", SqlType::TEXT, "end_to_end_test/data/c_name.kdb");
    table.Insert("c_address", SqlType::TEXT,
                 "end_to_end_test/data/c_address.kdb");
    table.Insert("c_nationkey", SqlType::INT,
                 "end_to_end_test/data/c_nationkey.kdb");
    table.Insert("c_phone", SqlType::TEXT, "end_to_end_test/data/c_phone.kdb");
    table.Insert("c_acctbal", SqlType::REAL,
                 "end_to_end_test/data/c_acctbal.kdb");
    table.Insert("c_mktsegment", SqlType::TEXT,
                 "end_to_end_test/data/c_mktsegment.kdb");
    table.Insert("c_comment", SqlType::TEXT,
                 "end_to_end_test/data/c_comment.kdb");
  }

  {
    auto& table = db.Insert("orders");
    table.Insert("o_orderkey", SqlType::INT,
                 "end_to_end_test/data/o_orderkey.kdb");
    table.Insert("o_custkey", SqlType::INT,
                 "end_to_end_test/data/o_custkey.kdb");
    table.Insert("o_orderstatus", SqlType::TEXT,
                 "end_to_end_test/data/o_orderstatus.kdb");
    table.Insert("o_totalprice", SqlType::REAL,
                 "end_to_end_test/data/o_totalprice.kdb");
    table.Insert("o_orderdate", SqlType::DATE,
                 "end_to_end_test/data/o_orderdate.kdb");
    table.Insert("o_orderpriority", SqlType::TEXT,
                 "end_to_end_test/data/o_orderpriority.kdb");
    table.Insert("o_clerk", SqlType::TEXT, "end_to_end_test/data/o_clerk.kdb");
    table.Insert("o_shippriority", SqlType::INT,
                 "end_to_end_test/data/o_shippriority.kdb");
  }

  {
    auto& table = db.Insert("nation");
    table.Insert("n_nationkey", SqlType::INT,
                 "end_to_end_test/data/n_nationkey.kdb");
    table.Insert("n_name", SqlType::TEXT, "end_to_end_test/data/n_name.kdb");
    table.Insert("n_regionkey", SqlType::INT,
                 "end_to_end_test/data/n_regionkey.kdb");
    table.Insert("n_comment", SqlType::TEXT,
                 "end_to_end_test/data/n_comment.kdb");
  }

  {
    auto& table = db.Insert("region");
    table.Insert("r_regionkey", SqlType::INT,
                 "end_to_end_test/data/r_regionkey.kdb");
    table.Insert("r_name", SqlType::TEXT, "end_to_end_test/data/r_name.kdb");
    table.Insert("r_comment", SqlType::TEXT,
                 "end_to_end_test/data/r_comment.kdb");
  }

  return db;
}