#pragma once

#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "runtime/enum.h"

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
    table.Insert("id", Type::Int(), "end_to_end_test/data/people_id.kdb",
                 "end_to_end_test/data/people_id_null.kdb",
                 "end_to_end_test/data/people_id.kdbindex");
    table.Insert("name", Type::Text(), "end_to_end_test/data/people_name.kdb",
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
    table.Insert("id", Type::Int(), "end_to_end_test/data/info_id.kdb",
                 "end_to_end_test/data/info_id_null.kdb",
                 "end_to_end_test/data/info_id.kdbindex");
    table.Insert("cheated", Type::Boolean(),
                 "end_to_end_test/data/info_cheated.kdb",
                 "end_to_end_test/data/info_cheated_null.kdb",
                 "end_to_end_test/data/info_cheated.kdbindex");
    table.Insert("date", Type::Date(), "end_to_end_test/data/info_date.kdb",
                 "end_to_end_test/data/info_date_null.kdb",
                 "end_to_end_test/data/info_date.kdbindex");
    table.Insert("zscore", Type::Real(), "end_to_end_test/data/info_zscore.kdb",
                 "end_to_end_test/data/info_zscore_null.kdb",
                 "end_to_end_test/data/info_zscore.kdbindex");
    table.Insert("num1", Type::SmallInt(), "end_to_end_test/data/info_num1.kdb",
                 "end_to_end_test/data/info_num1_null.kdb",
                 "end_to_end_test/data/info_num1.kdbindex");
    table.Insert("num2", Type::BigInt(), "end_to_end_test/data/info_num2.kdb",
                 "end_to_end_test/data/info_num2_null.kdb",
                 "end_to_end_test/data/info_num2.kdbindex");
  }

  {
    auto& table = db.Insert("supplier");
    table.Insert("s_suppkey", Type::Int(), "end_to_end_test/data/s_suppkey.kdb",
                 "", "end_to_end_test/data/s_suppkey.kdbindex");
    table.Insert("s_name", Type::Text(), "end_to_end_test/data/s_name.kdb", "",
                 "end_to_end_test/data/s_name.kdbindex");
    table.Insert("s_address", Type::Text(),
                 "end_to_end_test/data/s_address.kdb", "",
                 "end_to_end_test/data/s_address.kdbindex");
    table.Insert("s_nationkey", Type::Int(),
                 "end_to_end_test/data/s_nationkey.kdb", "",
                 "end_to_end_test/data/s_nationkey.kdbindex");
    table.Insert("s_phone", Type::Text(), "end_to_end_test/data/s_phone.kdb",
                 "", "end_to_end_test/data/s_phone.kdbindex");
    table.Insert("s_acctbal", Type::Real(),
                 "end_to_end_test/data/s_acctbal.kdb", "",
                 "end_to_end_test/data/s_acctbal.kdbindex");
    table.Insert("s_comment", Type::Text(),
                 "end_to_end_test/data/s_comment.kdb", "",
                 "end_to_end_test/data/s_comment.kdbindex");
  }

  {
    auto& table = db.Insert("part");
    table.Insert("p_partkey", Type::Int(), "end_to_end_test/data/p_partkey.kdb",
                 "", "end_to_end_test/data/p_partkey.kdbindex");
    table.Insert("p_name", Type::Text(), "end_to_end_test/data/p_name.kdb", "",
                 "end_to_end_test/data/p_name.kdbindex");
    table.Insert("p_mfgr", Type::Text(), "end_to_end_test/data/p_mfgr.kdb", "",
                 "end_to_end_test/data/p_mfgr.kdbindex");
    table.Insert("p_brand", Type::Text(), "end_to_end_test/data/p_brand.kdb",
                 "", "end_to_end_test/data/p_brand.kdbindex");
    table.Insert("p_type", Type::Text(), "end_to_end_test/data/p_type.kdb", "",
                 "end_to_end_test/data/p_type.kdbindex");
    table.Insert("p_size", Type::Int(), "end_to_end_test/data/p_size.kdb", "",
                 "end_to_end_test/data/p_size.kdbindex");
    table.Insert("p_container", Type::Text(),
                 "end_to_end_test/data/p_container.kdb", "",
                 "end_to_end_test/data/p_container.kdbindex");
    table.Insert("p_retailprice", Type::Real(),
                 "end_to_end_test/data/p_retailprice.kdb", "",
                 "end_to_end_test/data/p_retailprice.kdbindex");
    table.Insert("p_comment", Type::Text(),
                 "end_to_end_test/data/p_comment.kdb", "",
                 "end_to_end_test/data/p_comment.kdbindex");
  }

  {
    auto& table = db.Insert("partsupp");
    table.Insert("ps_partkey", Type::Int(),
                 "end_to_end_test/data/ps_partkey.kdb", "",
                 "end_to_end_test/data/ps_partkey.kdbindex");
    table.Insert("ps_suppkey", Type::Int(),
                 "end_to_end_test/data/ps_suppkey.kdb", "",
                 "end_to_end_test/data/ps_suppkey.kdbindex");
    table.Insert("ps_availqty", Type::Int(),
                 "end_to_end_test/data/ps_availqty.kdb", "",
                 "end_to_end_test/data/ps_availqty.kdbindex");
    table.Insert("ps_supplycost", Type::Real(),
                 "end_to_end_test/data/ps_supplycost.kdb", "",
                 "end_to_end_test/data/ps_supplycost.kdbindex");
    table.Insert("ps_comment", Type::Text(),
                 "end_to_end_test/data/ps_comment.kdb", "",
                 "end_to_end_test/data/ps_comment.kdbindex");
  }

  {
    auto& table = db.Insert("customer");
    table.Insert("c_custkey", Type::Int(), "end_to_end_test/data/c_custkey.kdb",
                 "", "end_to_end_test/data/c_custkey.kdbindex");
    table.Insert("c_name", Type::Text(), "end_to_end_test/data/c_name.kdb", "",
                 "end_to_end_test/data/c_name.kdbindex");
    table.Insert("c_address", Type::Text(),
                 "end_to_end_test/data/c_address.kdb", "",
                 "end_to_end_test/data/c_address.kdbindex");
    table.Insert("c_nationkey", Type::Int(),
                 "end_to_end_test/data/c_nationkey.kdb", "",
                 "end_to_end_test/data/c_nationkey.kdbindex");
    table.Insert("c_phone", Type::Text(), "end_to_end_test/data/c_phone.kdb",
                 "", "end_to_end_test/data/c_phone.kdbindex");
    table.Insert("c_acctbal", Type::Real(),
                 "end_to_end_test/data/c_acctbal.kdb", "",
                 "end_to_end_test/data/c_acctbal.kdbindex");
    table.Insert("c_mktsegment", Type::Text(),
                 "end_to_end_test/data/c_mktsegment.kdb", "",
                 "end_to_end_test/data/c_mktsegment.kdbindex");
    table.Insert("c_comment", Type::Text(),
                 "end_to_end_test/data/c_comment.kdb", "",
                 "end_to_end_test/data/c_comment.kdbindex");
  }

  {
    auto& table = db.Insert("orders");
    table.Insert("o_orderkey", Type::Int(),
                 "end_to_end_test/data/o_orderkey.kdb", "",
                 "end_to_end_test/data/o_orderkey.kdbindex");
    table.Insert("o_custkey", Type::Int(), "end_to_end_test/data/o_custkey.kdb",
                 "", "end_to_end_test/data/o_custkey.kdbindex");
    table.Insert("o_orderstatus", Type::Text(),
                 "end_to_end_test/data/o_orderstatus.kdb", "",
                 "end_to_end_test/data/o_orderstatus.kdbindex");
    table.Insert("o_totalprice", Type::Real(),
                 "end_to_end_test/data/o_totalprice.kdb", "",
                 "end_to_end_test/data/o_totalprice.kdbindex");
    table.Insert("o_orderdate", Type::Date(),
                 "end_to_end_test/data/o_orderdate.kdb", "",
                 "end_to_end_test/data/o_orderdate.kdbindex");
    table.Insert("o_orderpriority", Type::Text(),
                 "end_to_end_test/data/o_orderpriority.kdb", "",
                 "end_to_end_test/data/o_orderpriority.kdbindex");
    table.Insert("o_clerk", Type::Text(), "end_to_end_test/data/o_clerk.kdb",
                 "", "end_to_end_test/data/o_clerk.kdbindex");
    table.Insert("o_shippriority", Type::Int(),
                 "end_to_end_test/data/o_shippriority.kdb", "",
                 "end_to_end_test/data/o_shippriority.kdbindex");
    table.Insert("o_comment", Type::Text(),
                 "end_to_end_test/data/o_comment.kdb", "",
                 "end_to_end_test/data/o_comment.kdbindex");
  }

  {
    auto& table = db.Insert("lineitem");
    table.Insert("l_orderkey", Type::Int(),
                 "end_to_end_test/data/l_orderkey.kdb", "",
                 "end_to_end_test/data/l_orderkey.kdbindex");
    table.Insert("l_partkey", Type::Int(), "end_to_end_test/data/l_partkey.kdb",
                 "", "end_to_end_test/data/l_partkey.kdbindex");
    table.Insert("l_suppkey", Type::Int(), "end_to_end_test/data/l_suppkey.kdb",
                 "", "end_to_end_test/data/l_suppkey.kdbindex");
    table.Insert("l_linenumber", Type::Int(),
                 "end_to_end_test/data/l_linenumber.kdb", "",
                 "end_to_end_test/data/l_linenumber.kdbindex");
    table.Insert("l_quantity", Type::Real(),
                 "end_to_end_test/data/l_quantity.kdb", "",
                 "end_to_end_test/data/l_quantity.kdbindex");
    table.Insert("l_extendedprice", Type::Real(),
                 "end_to_end_test/data/l_extendedprice.kdb", "",
                 "end_to_end_test/data/l_extendedprice.kdbindex");
    table.Insert("l_discount", Type::Real(),
                 "end_to_end_test/data/l_discount.kdb", "",
                 "end_to_end_test/data/l_discount.kdbindex");
    table.Insert("l_tax", Type::Real(), "end_to_end_test/data/l_tax.kdb", "",
                 "end_to_end_test/data/l_tax.kdbindex");
    table.Insert("l_returnflag", Type::Text(),
                 "end_to_end_test/data/l_returnflag.kdb", "",
                 "end_to_end_test/data/l_returnflag.kdbindex");
    table.Insert("l_linestatus", Type::Text(),
                 "end_to_end_test/data/l_linestatus.kdb", "",
                 "end_to_end_test/data/l_linestatus.kdbindex");
    table.Insert("l_shipdate", Type::Date(),
                 "end_to_end_test/data/l_shipdate.kdb", "",
                 "end_to_end_test/data/l_shipdate.kdbindex");
    table.Insert("l_commitdate", Type::Date(),
                 "end_to_end_test/data/l_commitdate.kdb", "",
                 "end_to_end_test/data/l_commitdate.kdbindex");
    table.Insert("l_receiptdate", Type::Date(),
                 "end_to_end_test/data/l_receiptdate.kdb", "",
                 "end_to_end_test/data/l_receiptdate.kdbindex");
    table.Insert("l_shipinstruct", Type::Text(),
                 "end_to_end_test/data/l_shipinstruct.kdb", "",
                 "end_to_end_test/data/l_shipinstruct.kdbindex");
    table.Insert("l_shipmode", Type::Text(),
                 "end_to_end_test/data/l_shipmode.kdb", "",
                 "end_to_end_test/data/l_shipmode.kdbindex");
    table.Insert("l_comment", Type::Text(),
                 "end_to_end_test/data/l_comment.kdb", "",
                 "end_to_end_test/data/l_comment.kdbindex");
  }

  {
    auto& table = db.Insert("nation");
    table.Insert("n_nationkey", Type::Int(),
                 "end_to_end_test/data/n_nationkey.kdb", "",
                 "end_to_end_test/data/n_nationkey.kdbindex");
    table.Insert("n_name", Type::Text(), "end_to_end_test/data/n_name.kdb", "",
                 "end_to_end_test/data/n_name.kdbindex");
    table.Insert("n_regionkey", Type::Int(),
                 "end_to_end_test/data/n_regionkey.kdb", "",
                 "end_to_end_test/data/n_regionkey.kdbindex");
    table.Insert("n_comment", Type::Text(),
                 "end_to_end_test/data/n_comment.kdb", "",
                 "end_to_end_test/data/n_comment.kdbindex");
  }

  {
    auto& table = db.Insert("region");
    table.Insert("r_regionkey", Type::Int(),
                 "end_to_end_test/data/r_regionkey.kdb", "",
                 "end_to_end_test/data/r_regionkey.kdbindex");
    table.Insert("r_name", Type::Text(), "end_to_end_test/data/r_name.kdb", "",
                 "end_to_end_test/data/r_name.kdbindex");
    table.Insert("r_comment", Type::Text(),
                 "end_to_end_test/data/r_comment.kdb", "",
                 "end_to_end_test/data/r_comment.kdbindex");
  }

  return db;
}

kush::catalog::Database EnumSchema() {
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
    table.Insert("id", Type::Int(), "end_to_end_test/data/people_id.kdb",
                 "end_to_end_test/data/people_id_null.kdb",
                 "end_to_end_test/data/people_id.kdbindex");

    auto name_id = kush::runtime::Enum::EnumManager::Get().Register(
        "end_to_end_test/data/people_name_enum.kdbenum");
    table.Insert("name", Type::Enum(name_id),
                 "end_to_end_test/data/people_name_enum.kdb",
                 "end_to_end_test/data/people_name_enum_null.kdb",
                 "end_to_end_test/data/people_name_enum.kdbindex");
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
    table.Insert("id", Type::Int(), "end_to_end_test/data/info_id.kdb",
                 "end_to_end_test/data/info_id_null.kdb",
                 "end_to_end_test/data/info_id.kdbindex");
    table.Insert("cheated", Type::Boolean(),
                 "end_to_end_test/data/info_cheated.kdb",
                 "end_to_end_test/data/info_cheated_null.kdb",
                 "end_to_end_test/data/info_cheated.kdbindex");
    table.Insert("date", Type::Date(), "end_to_end_test/data/info_date.kdb",
                 "end_to_end_test/data/info_date_null.kdb",
                 "end_to_end_test/data/info_date.kdbindex");
    table.Insert("zscore", Type::Real(), "end_to_end_test/data/info_zscore.kdb",
                 "end_to_end_test/data/info_zscore_null.kdb",
                 "end_to_end_test/data/info_zscore.kdbindex");
    table.Insert("num1", Type::SmallInt(), "end_to_end_test/data/info_num1.kdb",
                 "end_to_end_test/data/info_num1_null.kdb",
                 "end_to_end_test/data/info_num1.kdbindex");
    table.Insert("num2", Type::BigInt(), "end_to_end_test/data/info_num2.kdb",
                 "end_to_end_test/data/info_num2_null.kdb",
                 "end_to_end_test/data/info_num2.kdbindex");
  }

  {
    auto& table = db.Insert("supplier");
    table.Insert("s_suppkey", Type::Int(), "end_to_end_test/data/s_suppkey.kdb",
                 "", "end_to_end_test/data/s_suppkey.kdbindex");
    table.Insert("s_name", Type::Text(), "end_to_end_test/data/s_name.kdb", "",
                 "end_to_end_test/data/s_name.kdbindex");
    table.Insert("s_address", Type::Text(),
                 "end_to_end_test/data/s_address.kdb", "",
                 "end_to_end_test/data/s_address.kdbindex");
    table.Insert("s_nationkey", Type::Int(),
                 "end_to_end_test/data/s_nationkey.kdb", "",
                 "end_to_end_test/data/s_nationkey.kdbindex");
    table.Insert("s_phone", Type::Text(), "end_to_end_test/data/s_phone.kdb",
                 "", "end_to_end_test/data/s_phone.kdbindex");
    table.Insert("s_acctbal", Type::Real(),
                 "end_to_end_test/data/s_acctbal.kdb", "",
                 "end_to_end_test/data/s_acctbal.kdbindex");
    table.Insert("s_comment", Type::Text(),
                 "end_to_end_test/data/s_comment.kdb", "",
                 "end_to_end_test/data/s_comment.kdbindex");
  }

  {
    auto& table = db.Insert("part");
    table.Insert("p_partkey", Type::Int(), "end_to_end_test/data/p_partkey.kdb",
                 "", "end_to_end_test/data/p_partkey.kdbindex");
    table.Insert("p_name", Type::Text(), "end_to_end_test/data/p_name.kdb", "",
                 "end_to_end_test/data/p_name.kdbindex");
    table.Insert("p_mfgr", Type::Text(), "end_to_end_test/data/p_mfgr.kdb", "",
                 "end_to_end_test/data/p_mfgr.kdbindex");
    table.Insert("p_brand", Type::Text(), "end_to_end_test/data/p_brand.kdb",
                 "", "end_to_end_test/data/p_brand.kdbindex");
    table.Insert("p_type", Type::Text(), "end_to_end_test/data/p_type.kdb", "",
                 "end_to_end_test/data/p_type.kdbindex");
    table.Insert("p_size", Type::Int(), "end_to_end_test/data/p_size.kdb", "",
                 "end_to_end_test/data/p_size.kdbindex");
    table.Insert("p_container", Type::Text(),
                 "end_to_end_test/data/p_container.kdb", "",
                 "end_to_end_test/data/p_container.kdbindex");
    table.Insert("p_retailprice", Type::Real(),
                 "end_to_end_test/data/p_retailprice.kdb", "",
                 "end_to_end_test/data/p_retailprice.kdbindex");
    table.Insert("p_comment", Type::Text(),
                 "end_to_end_test/data/p_comment.kdb", "",
                 "end_to_end_test/data/p_comment.kdbindex");
  }

  {
    auto& table = db.Insert("partsupp");
    table.Insert("ps_partkey", Type::Int(),
                 "end_to_end_test/data/ps_partkey.kdb", "",
                 "end_to_end_test/data/ps_partkey.kdbindex");
    table.Insert("ps_suppkey", Type::Int(),
                 "end_to_end_test/data/ps_suppkey.kdb", "",
                 "end_to_end_test/data/ps_suppkey.kdbindex");
    table.Insert("ps_availqty", Type::Int(),
                 "end_to_end_test/data/ps_availqty.kdb", "",
                 "end_to_end_test/data/ps_availqty.kdbindex");
    table.Insert("ps_supplycost", Type::Real(),
                 "end_to_end_test/data/ps_supplycost.kdb", "",
                 "end_to_end_test/data/ps_supplycost.kdbindex");
    table.Insert("ps_comment", Type::Text(),
                 "end_to_end_test/data/ps_comment.kdb", "",
                 "end_to_end_test/data/ps_comment.kdbindex");
  }

  {
    auto& table = db.Insert("customer");
    table.Insert("c_custkey", Type::Int(), "end_to_end_test/data/c_custkey.kdb",
                 "", "end_to_end_test/data/c_custkey.kdbindex");
    table.Insert("c_name", Type::Text(), "end_to_end_test/data/c_name.kdb", "",
                 "end_to_end_test/data/c_name.kdbindex");
    table.Insert("c_address", Type::Text(),
                 "end_to_end_test/data/c_address.kdb", "",
                 "end_to_end_test/data/c_address.kdbindex");
    table.Insert("c_nationkey", Type::Int(),
                 "end_to_end_test/data/c_nationkey.kdb", "",
                 "end_to_end_test/data/c_nationkey.kdbindex");
    table.Insert("c_phone", Type::Text(), "end_to_end_test/data/c_phone.kdb",
                 "", "end_to_end_test/data/c_phone.kdbindex");
    table.Insert("c_acctbal", Type::Real(),
                 "end_to_end_test/data/c_acctbal.kdb", "",
                 "end_to_end_test/data/c_acctbal.kdbindex");
    table.Insert("c_mktsegment", Type::Text(),
                 "end_to_end_test/data/c_mktsegment.kdb", "",
                 "end_to_end_test/data/c_mktsegment.kdbindex");
    table.Insert("c_comment", Type::Text(),
                 "end_to_end_test/data/c_comment.kdb", "",
                 "end_to_end_test/data/c_comment.kdbindex");
  }

  {
    auto& table = db.Insert("orders");
    table.Insert("o_orderkey", Type::Int(),
                 "end_to_end_test/data/o_orderkey.kdb", "",
                 "end_to_end_test/data/o_orderkey.kdbindex");
    table.Insert("o_custkey", Type::Int(), "end_to_end_test/data/o_custkey.kdb",
                 "", "end_to_end_test/data/o_custkey.kdbindex");
    table.Insert("o_orderstatus", Type::Text(),
                 "end_to_end_test/data/o_orderstatus.kdb", "",
                 "end_to_end_test/data/o_orderstatus.kdbindex");
    table.Insert("o_totalprice", Type::Real(),
                 "end_to_end_test/data/o_totalprice.kdb", "",
                 "end_to_end_test/data/o_totalprice.kdbindex");
    table.Insert("o_orderdate", Type::Date(),
                 "end_to_end_test/data/o_orderdate.kdb", "",
                 "end_to_end_test/data/o_orderdate.kdbindex");
    table.Insert("o_orderpriority", Type::Text(),
                 "end_to_end_test/data/o_orderpriority.kdb", "",
                 "end_to_end_test/data/o_orderpriority.kdbindex");
    table.Insert("o_clerk", Type::Text(), "end_to_end_test/data/o_clerk.kdb",
                 "", "end_to_end_test/data/o_clerk.kdbindex");
    table.Insert("o_shippriority", Type::Int(),
                 "end_to_end_test/data/o_shippriority.kdb", "",
                 "end_to_end_test/data/o_shippriority.kdbindex");
    table.Insert("o_comment", Type::Text(),
                 "end_to_end_test/data/o_comment.kdb", "",
                 "end_to_end_test/data/o_comment.kdbindex");
  }

  {
    auto& table = db.Insert("lineitem");
    table.Insert("l_orderkey", Type::Int(),
                 "end_to_end_test/data/l_orderkey.kdb", "",
                 "end_to_end_test/data/l_orderkey.kdbindex");
    table.Insert("l_partkey", Type::Int(), "end_to_end_test/data/l_partkey.kdb",
                 "", "end_to_end_test/data/l_partkey.kdbindex");
    table.Insert("l_suppkey", Type::Int(), "end_to_end_test/data/l_suppkey.kdb",
                 "", "end_to_end_test/data/l_suppkey.kdbindex");
    table.Insert("l_linenumber", Type::Int(),
                 "end_to_end_test/data/l_linenumber.kdb", "",
                 "end_to_end_test/data/l_linenumber.kdbindex");
    table.Insert("l_quantity", Type::Real(),
                 "end_to_end_test/data/l_quantity.kdb", "",
                 "end_to_end_test/data/l_quantity.kdbindex");
    table.Insert("l_extendedprice", Type::Real(),
                 "end_to_end_test/data/l_extendedprice.kdb", "",
                 "end_to_end_test/data/l_extendedprice.kdbindex");
    table.Insert("l_discount", Type::Real(),
                 "end_to_end_test/data/l_discount.kdb", "",
                 "end_to_end_test/data/l_discount.kdbindex");
    table.Insert("l_tax", Type::Real(), "end_to_end_test/data/l_tax.kdb", "",
                 "end_to_end_test/data/l_tax.kdbindex");
    table.Insert("l_returnflag", Type::Text(),
                 "end_to_end_test/data/l_returnflag.kdb", "",
                 "end_to_end_test/data/l_returnflag.kdbindex");
    table.Insert("l_linestatus", Type::Text(),
                 "end_to_end_test/data/l_linestatus.kdb", "",
                 "end_to_end_test/data/l_linestatus.kdbindex");
    table.Insert("l_shipdate", Type::Date(),
                 "end_to_end_test/data/l_shipdate.kdb", "",
                 "end_to_end_test/data/l_shipdate.kdbindex");
    table.Insert("l_commitdate", Type::Date(),
                 "end_to_end_test/data/l_commitdate.kdb", "",
                 "end_to_end_test/data/l_commitdate.kdbindex");
    table.Insert("l_receiptdate", Type::Date(),
                 "end_to_end_test/data/l_receiptdate.kdb", "",
                 "end_to_end_test/data/l_receiptdate.kdbindex");
    table.Insert("l_shipinstruct", Type::Text(),
                 "end_to_end_test/data/l_shipinstruct.kdb", "",
                 "end_to_end_test/data/l_shipinstruct.kdbindex");
    table.Insert("l_shipmode", Type::Text(),
                 "end_to_end_test/data/l_shipmode.kdb", "",
                 "end_to_end_test/data/l_shipmode.kdbindex");
    table.Insert("l_comment", Type::Text(),
                 "end_to_end_test/data/l_comment.kdb", "",
                 "end_to_end_test/data/l_comment.kdbindex");
  }

  {
    auto& table = db.Insert("nation");
    table.Insert("n_nationkey", Type::Int(),
                 "end_to_end_test/data/n_nationkey.kdb", "",
                 "end_to_end_test/data/n_nationkey.kdbindex");
    table.Insert("n_name", Type::Text(), "end_to_end_test/data/n_name.kdb", "",
                 "end_to_end_test/data/n_name.kdbindex");
    table.Insert("n_regionkey", Type::Int(),
                 "end_to_end_test/data/n_regionkey.kdb", "",
                 "end_to_end_test/data/n_regionkey.kdbindex");
    table.Insert("n_comment", Type::Text(),
                 "end_to_end_test/data/n_comment.kdb", "",
                 "end_to_end_test/data/n_comment.kdbindex");
  }

  {
    auto& table = db.Insert("region");
    table.Insert("r_regionkey", Type::Int(),
                 "end_to_end_test/data/r_regionkey.kdb", "",
                 "end_to_end_test/data/r_regionkey.kdbindex");
    table.Insert("r_name", Type::Text(), "end_to_end_test/data/r_name.kdb", "",
                 "end_to_end_test/data/r_name.kdbindex");
    table.Insert("r_comment", Type::Text(),
                 "end_to_end_test/data/r_comment.kdb", "",
                 "end_to_end_test/data/r_comment.kdbindex");
  }

  return db;
}