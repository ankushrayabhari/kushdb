#pragma once

#include "catalog/catalog.h"
#include "catalog/sql_type.h"

kush::catalog::Database Schema() {
  using namespace kush::catalog;
  Database db;

  {
    /*
      CREATE TABLE people (
        id INTEGER,
        name TEXT,
      );
    */
    auto& table = db.Insert("people");
    table.Insert("id", SqlType::INT, "end_to_end_test/data/people_id.kdb",
                 "end_to_end_test/data/people_id_null.kdb",
                 "end_to_end_test/data/people_id.kdbindex");
    table.Insert("name", SqlType::TEXT, "end_to_end_test/data/people_name.kdb",
                 "end_to_end_test/data/people_name_null.kdb",
                 "end_to_end_test/data/people_name.kdbindex");
  }

  {
    /*
      CREATE TABLE info (
        id INTEGER,
        cheated BOOLEAN,
        date DATE,
        zscore REAL,
        num1 SMALLINT,
        num2 BIGINT,
      );
    */
    auto& table = db.Insert("info");
    table.Insert("id", SqlType::INT, "end_to_end_test/data/info_id.kdb",
                 "end_to_end_test/data/info_id_null.kdb",
                 "end_to_end_test/data/info_id.kdbindex");
    table.Insert("cheated", SqlType::BOOLEAN,
                 "end_to_end_test/data/info_cheated.kdb",
                 "end_to_end_test/data/info_cheated_null.kdb",
                 "end_to_end_test/data/info_cheated.kdbindex");
    table.Insert("date", SqlType::DATE, "end_to_end_test/data/info_date.kdb",
                 "end_to_end_test/data/info_date_null.kdb",
                 "end_to_end_test/data/info_date.kdbindex");
    table.Insert("zscore", SqlType::REAL,
                 "end_to_end_test/data/info_zscore.kdb",
                 "end_to_end_test/data/info_zscore_null.kdb",
                 "end_to_end_test/data/info_zscore.kdbindex");
    table.Insert("num1", SqlType::SMALLINT,
                 "end_to_end_test/data/info_num1.kdb",
                 "end_to_end_test/data/info_num1_null.kdb",
                 "end_to_end_test/data/info_num1.kdbindex");
    table.Insert("num2", SqlType::BIGINT, "end_to_end_test/data/info_num2.kdb",
                 "end_to_end_test/data/info_num2_null.kdb",
                 "end_to_end_test/data/info_num2.kdbindex");
  }

  {
    auto& table = db.Insert("supplier");
    table.Insert("s_suppkey", SqlType::INT,
                 "end_to_end_test/data/s_suppkey.kdb", "",
                 "end_to_end_test/data/s_suppkey.kdbindex");
    table.Insert("s_name", SqlType::TEXT, "end_to_end_test/data/s_name.kdb", "",
                 "end_to_end_test/data/s_name.kdbindex");
    table.Insert("s_address", SqlType::TEXT,
                 "end_to_end_test/data/s_address.kdb", "",
                 "end_to_end_test/data/s_address.kdbindex");
    table.Insert("s_nationkey", SqlType::INT,
                 "end_to_end_test/data/s_nationkey.kdb", "",
                 "end_to_end_test/data/s_nationkey.kdbindex");
    table.Insert("s_phone", SqlType::TEXT, "end_to_end_test/data/s_phone.kdb",
                 "", "end_to_end_test/data/s_phone.kdbindex");
    table.Insert("s_acctbal", SqlType::REAL,
                 "end_to_end_test/data/s_acctbal.kdb", "",
                 "end_to_end_test/data/s_acctbal.kdbindex");
    table.Insert("s_comment", SqlType::TEXT,
                 "end_to_end_test/data/s_comment.kdb", "",
                 "end_to_end_test/data/s_comment.kdbindex");
  }

  {
    auto& table = db.Insert("part");
    table.Insert("p_partkey", SqlType::INT,
                 "end_to_end_test/data/p_partkey.kdb", "",
                 "end_to_end_test/data/p_partkey.kdbindex");
    table.Insert("p_name", SqlType::TEXT, "end_to_end_test/data/p_name.kdb", "",
                 "end_to_end_test/data/p_name.kdbindex");
    table.Insert("p_mfgr", SqlType::TEXT, "end_to_end_test/data/p_mfgr.kdb", "",
                 "end_to_end_test/data/p_mfgr.kdbindex");
    table.Insert("p_brand", SqlType::TEXT, "end_to_end_test/data/p_brand.kdb",
                 "", "end_to_end_test/data/p_brand.kdbindex");
    table.Insert("p_type", SqlType::TEXT, "end_to_end_test/data/p_type.kdb", "",
                 "end_to_end_test/data/p_type.kdbindex");
    table.Insert("p_size", SqlType::INT, "end_to_end_test/data/p_size.kdb", "",
                 "end_to_end_test/data/p_size.kdbindex");
    table.Insert("p_container", SqlType::TEXT,
                 "end_to_end_test/data/p_container.kdb", "",
                 "end_to_end_test/data/p_container.kdbindex");
    table.Insert("p_retailprice", SqlType::REAL,
                 "end_to_end_test/data/p_retailprice.kdb", "",
                 "end_to_end_test/data/p_retailprice.kdbindex");
    table.Insert("p_comment", SqlType::TEXT,
                 "end_to_end_test/data/p_comment.kdb", "",
                 "end_to_end_test/data/p_comment.kdbindex");
  }

  {
    auto& table = db.Insert("partsupp");
    table.Insert("ps_partkey", SqlType::INT,
                 "end_to_end_test/data/ps_partkey.kdb", "",
                 "end_to_end_test/data/ps_partkey.kdbindex");
    table.Insert("ps_suppkey", SqlType::INT,
                 "end_to_end_test/data/ps_suppkey.kdb", "",
                 "end_to_end_test/data/ps_suppkey.kdbindex");
    table.Insert("ps_availqty", SqlType::INT,
                 "end_to_end_test/data/ps_availqty.kdb", "",
                 "end_to_end_test/data/ps_availqty.kdbindex");
    table.Insert("ps_supplycost", SqlType::REAL,
                 "end_to_end_test/data/ps_supplycost.kdb", "",
                 "end_to_end_test/data/ps_supplycost.kdbindex");
    table.Insert("ps_comment", SqlType::TEXT,
                 "end_to_end_test/data/ps_comment.kdb", "",
                 "end_to_end_test/data/ps_comment.kdbindex");
  }

  {
    auto& table = db.Insert("customer");
    table.Insert("c_custkey", SqlType::INT,
                 "end_to_end_test/data/c_custkey.kdb", "",
                 "end_to_end_test/data/c_custkey.kdbindex");
    table.Insert("c_name", SqlType::TEXT, "end_to_end_test/data/c_name.kdb", "",
                 "end_to_end_test/data/c_name.kdbindex");
    table.Insert("c_address", SqlType::TEXT,
                 "end_to_end_test/data/c_address.kdb", "",
                 "end_to_end_test/data/c_address.kdbindex");
    table.Insert("c_nationkey", SqlType::INT,
                 "end_to_end_test/data/c_nationkey.kdb", "",
                 "end_to_end_test/data/c_nationkey.kdbindex");
    table.Insert("c_phone", SqlType::TEXT, "end_to_end_test/data/c_phone.kdb",
                 "", "end_to_end_test/data/c_phone.kdbindex");
    table.Insert("c_acctbal", SqlType::REAL,
                 "end_to_end_test/data/c_acctbal.kdb", "",
                 "end_to_end_test/data/c_acctbal.kdbindex");
    table.Insert("c_mktsegment", SqlType::TEXT,
                 "end_to_end_test/data/c_mktsegment.kdb", "",
                 "end_to_end_test/data/c_mktsegment.kdbindex");
    table.Insert("c_comment", SqlType::TEXT,
                 "end_to_end_test/data/c_comment.kdb", "",
                 "end_to_end_test/data/c_comment.kdbindex");
  }

  {
    auto& table = db.Insert("orders");
    table.Insert("o_orderkey", SqlType::INT,
                 "end_to_end_test/data/o_orderkey.kdb", "",
                 "end_to_end_test/data/o_orderkey.kdbindex");
    table.Insert("o_custkey", SqlType::INT,
                 "end_to_end_test/data/o_custkey.kdb", "",
                 "end_to_end_test/data/o_custkey.kdbindex");
    table.Insert("o_orderstatus", SqlType::TEXT,
                 "end_to_end_test/data/o_orderstatus.kdb", "",
                 "end_to_end_test/data/o_orderstatus.kdbindex");
    table.Insert("o_totalprice", SqlType::REAL,
                 "end_to_end_test/data/o_totalprice.kdb", "",
                 "end_to_end_test/data/o_totalprice.kdbindex");
    table.Insert("o_orderdate", SqlType::DATE,
                 "end_to_end_test/data/o_orderdate.kdb", "",
                 "end_to_end_test/data/o_orderdate.kdbindex");
    table.Insert("o_orderpriority", SqlType::TEXT,
                 "end_to_end_test/data/o_orderpriority.kdb", "",
                 "end_to_end_test/data/o_orderpriority.kdbindex");
    table.Insert("o_clerk", SqlType::TEXT, "end_to_end_test/data/o_clerk.kdb",
                 "", "end_to_end_test/data/o_clerk.kdbindex");
    table.Insert("o_shippriority", SqlType::INT,
                 "end_to_end_test/data/o_shippriority.kdb", "",
                 "end_to_end_test/data/o_shippriority.kdbindex");
    table.Insert("o_comment", SqlType::TEXT,
                 "end_to_end_test/data/o_comment.kdb", "",
                 "end_to_end_test/data/o_comment.kdbindex");
  }

  {
    auto& table = db.Insert("lineitem");
    table.Insert("l_orderkey", SqlType::INT,
                 "end_to_end_test/data/l_orderkey.kdb", "",
                 "end_to_end_test/data/l_orderkey.kdbindex");
    table.Insert("l_partkey", SqlType::INT,
                 "end_to_end_test/data/l_partkey.kdb", "",
                 "end_to_end_test/data/l_partkey.kdbindex");
    table.Insert("l_suppkey", SqlType::INT,
                 "end_to_end_test/data/l_suppkey.kdb", "",
                 "end_to_end_test/data/l_suppkey.kdbindex");
    table.Insert("l_linenumber", SqlType::INT,
                 "end_to_end_test/data/l_linenumber.kdb", "",
                 "end_to_end_test/data/l_linenumber.kdbindex");
    table.Insert("l_quantity", SqlType::REAL,
                 "end_to_end_test/data/l_quantity.kdb", "",
                 "end_to_end_test/data/l_quantity.kdbindex");
    table.Insert("l_extendedprice", SqlType::REAL,
                 "end_to_end_test/data/l_extendedprice.kdb", "",
                 "end_to_end_test/data/l_extendedprice.kdbindex");
    table.Insert("l_discount", SqlType::REAL,
                 "end_to_end_test/data/l_discount.kdb", "",
                 "end_to_end_test/data/l_discount.kdbindex");
    table.Insert("l_tax", SqlType::REAL, "end_to_end_test/data/l_tax.kdb", "",
                 "end_to_end_test/data/l_tax.kdbindex");
    table.Insert("l_returnflag", SqlType::TEXT,
                 "end_to_end_test/data/l_returnflag.kdb", "",
                 "end_to_end_test/data/l_returnflag.kdbindex");
    table.Insert("l_linestatus", SqlType::TEXT,
                 "end_to_end_test/data/l_linestatus.kdb", "",
                 "end_to_end_test/data/l_linestatus.kdbindex");
    table.Insert("l_shipdate", SqlType::DATE,
                 "end_to_end_test/data/l_shipdate.kdb", "",
                 "end_to_end_test/data/l_shipdate.kdbindex");
    table.Insert("l_commitdate", SqlType::DATE,
                 "end_to_end_test/data/l_commitdate.kdb", "",
                 "end_to_end_test/data/l_commitdate.kdbindex");
    table.Insert("l_receiptdate", SqlType::DATE,
                 "end_to_end_test/data/l_receiptdate.kdb", "",
                 "end_to_end_test/data/l_receiptdate.kdbindex");
    table.Insert("l_shipinstruct", SqlType::TEXT,
                 "end_to_end_test/data/l_shipinstruct.kdb", "",
                 "end_to_end_test/data/l_shipinstruct.kdbindex");
    table.Insert("l_shipmode", SqlType::TEXT,
                 "end_to_end_test/data/l_shipmode.kdb", "",
                 "end_to_end_test/data/l_shipmode.kdbindex");
    table.Insert("l_comment", SqlType::TEXT,
                 "end_to_end_test/data/l_comment.kdb", "",
                 "end_to_end_test/data/l_comment.kdbindex");
  }

  {
    auto& table = db.Insert("nation");
    table.Insert("n_nationkey", SqlType::INT,
                 "end_to_end_test/data/n_nationkey.kdb", "",
                 "end_to_end_test/data/n_nationkey.kdbindex");
    table.Insert("n_name", SqlType::TEXT, "end_to_end_test/data/n_name.kdb", "",
                 "end_to_end_test/data/n_name.kdbindex");
    table.Insert("n_regionkey", SqlType::INT,
                 "end_to_end_test/data/n_regionkey.kdb", "",
                 "end_to_end_test/data/n_regionkey.kdbindex");
    table.Insert("n_comment", SqlType::TEXT,
                 "end_to_end_test/data/n_comment.kdb", "",
                 "end_to_end_test/data/n_comment.kdbindex");
  }

  {
    auto& table = db.Insert("region");
    table.Insert("r_regionkey", SqlType::INT,
                 "end_to_end_test/data/r_regionkey.kdb", "",
                 "end_to_end_test/data/r_regionkey.kdbindex");
    table.Insert("r_name", SqlType::TEXT, "end_to_end_test/data/r_name.kdb", "",
                 "end_to_end_test/data/r_name.kdbindex");
    table.Insert("r_comment", SqlType::TEXT,
                 "end_to_end_test/data/r_comment.kdb", "",
                 "end_to_end_test/data/r_comment.kdbindex");
  }

  return db;
}