#include <cstdint>
#include <execution>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>

#include "util/load.h"

using namespace kush::util;

std::string dest = "benchmark/jcch10/data/";
std::string raw = "benchmark/jcch10/raw/";

void Region() {
  /*
    CREATE TABLE region (
      r_regionkey INTEGER NOT NULL,
      r_name CHAR(25) NOT NULL,
      r_comment VARCHAR(152) NOT NULL
    );
  */

  DECLARE_NOT_NULL_ENUM_COL(r_name_enum);
  DECLARE_NOT_NULL_ENUM_COL(r_comment_enum);

  std::ifstream fin(raw + "region.tbl");
  int tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, '|', 3);

    APPEND_NOT_NULL_ENUM(r_name_enum, data[1], tuple_idx);
    APPEND_NOT_NULL_ENUM(r_comment_enum, data[2], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NOT_NULL_ENUM(r_name_enum, dest, "r_name_enum");
  SERIALIZE_NOT_NULL_ENUM(r_comment_enum, dest, "r_comment_enum");
  Print("region complete");
}

void Nation() {
  /*
    CREATE TABLE nation (
      n_nationkey INTEGER NOT NULL,
      n_name CHAR(25) NOT NULL,
      n_regionkey INTEGER NOT NULL,
      n_comment VARCHAR(152) NOT NULL
    );
  */

  DECLARE_NOT_NULL_ENUM_COL(n_name_enum);
  DECLARE_NOT_NULL_ENUM_COL(n_comment_enum);

  std::ifstream fin(raw + "nation.tbl");
  int tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, '|', 4);

    APPEND_NOT_NULL_ENUM(n_name_enum, data[1], tuple_idx);
    APPEND_NOT_NULL_ENUM(n_comment_enum, data[3], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NOT_NULL_ENUM(n_name_enum, dest, "n_name_enum");
  SERIALIZE_NOT_NULL_ENUM(n_comment_enum, dest, "n_comment_enum");
  Print("nation complete");
}

void Lineitem() {
  /*
    CREATE TABLE lineitem (
      l_orderkey INTEGER NOT NULL,
      l_partkey INTEGER NOT NULL,
      l_suppkey INTEGER NOT NULL,
      l_linenumber INTEGER NOT NULL,
      l_quantity NUMERIC NOT NULL,
      l_extendedprice NUMERIC NOT NULL,
      l_discount NUMERIC NOT NULL,
      l_tax NUMERIC NOT NULL,
      l_returnflag CHAR(1) NOT NULL,
      l_linestatus CHAR(1) NOT NULL,
      l_shipdate DATE NOT NULL,
      l_commitdate DATE NOT NULL,
      l_receiptdate DATE NOT NULL,
      l_shipinstruct CHAR(25) NOT NULL,
      l_shipmode CHAR(10) NOT NULL,
      l_comment VARCHAR(44) NOT NULL
    );
  */

  DECLARE_NOT_NULL_ENUM_COL(l_returnflag_enum);
  DECLARE_NOT_NULL_ENUM_COL(l_linestatus_enum);
  DECLARE_NOT_NULL_ENUM_COL(l_shipinstruct_enum);
  DECLARE_NOT_NULL_ENUM_COL(l_shipmode_enum);
  DECLARE_NOT_NULL_ENUM_COL(l_comment_enum);

  std::ifstream fin(raw + "lineitem.tbl");
  int32_t tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, '|', 16);

    APPEND_NOT_NULL_ENUM(l_returnflag_enum, data[8], tuple_idx);
    APPEND_NOT_NULL_ENUM(l_linestatus_enum, data[9], tuple_idx);
    APPEND_NOT_NULL_ENUM(l_shipinstruct_enum, data[13], tuple_idx);
    APPEND_NOT_NULL_ENUM(l_shipmode_enum, data[14], tuple_idx);
    APPEND_NOT_NULL_ENUM(l_comment_enum, data[15], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NOT_NULL_ENUM(l_returnflag_enum, dest, "l_returnflag_enum");
  SERIALIZE_NOT_NULL_ENUM(l_linestatus_enum, dest, "l_linestatus_enum");
  SERIALIZE_NOT_NULL_ENUM(l_shipinstruct_enum, dest, "l_shipinstruct_enum");
  SERIALIZE_NOT_NULL_ENUM(l_shipmode_enum, dest, "l_shipmode_enum");
  SERIALIZE_NOT_NULL_ENUM(l_comment_enum, dest, "l_comment_enum");
  Print("lineitem complete");
}

void Orders() {
  /*
    CREATE TABLE orders (
      o_orderkey INTEGER NOT NULL,
      o_custkey INTEGER NOT NULL,
      o_orderstatus CHAR(1) NOT NULL,
      o_totalprice NUMERIC NOT NULL,
      o_orderdate DATE NOT NULL,
      o_orderpriority CHAR(15) NOT NULL,
      o_clerk CHAR(15) NOT NULL,
      o_shippriority INTEGER NOT NULL,
      o_comment VARCHAR(79) NOT NULL
    );
  */

  DECLARE_NOT_NULL_ENUM_COL(o_orderstatus_enum);
  DECLARE_NOT_NULL_ENUM_COL(o_orderpriority_enum);
  DECLARE_NOT_NULL_ENUM_COL(o_clerk_enum);
  DECLARE_NOT_NULL_ENUM_COL(o_comment_enum);

  std::ifstream fin(raw + "orders.tbl");
  int32_t tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, '|', 9);

    APPEND_NOT_NULL_ENUM(o_orderstatus_enum, data[2], tuple_idx);
    APPEND_NOT_NULL_ENUM(o_orderpriority_enum, data[5], tuple_idx);
    APPEND_NOT_NULL_ENUM(o_clerk_enum, data[6], tuple_idx);
    APPEND_NOT_NULL_ENUM(o_comment_enum, data[8], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NOT_NULL_ENUM(o_orderstatus_enum, dest, "o_orderstatus_enum");
  SERIALIZE_NOT_NULL_ENUM(o_orderpriority_enum, dest, "o_orderpriority_enum");
  SERIALIZE_NOT_NULL_ENUM(o_clerk_enum, dest, "o_clerk_enum");
  SERIALIZE_NOT_NULL_ENUM(o_comment_enum, dest, "o_comment_enum");
  Print("orders complete");
}

void Customer() {
  /*
    CREATE TABLE customer (
      c_custkey INTEGER NOT NULL,
      c_name VARCHAR(25) NOT NULL,
      c_address VARCHAR(40) NOT NULL,
      c_nationkey INTEGER NOT NULL,
      c_phone CHAR(15) NOT NULL,
      c_acctbal NUMERIC NOT NULL,
      c_mktsegment CHAR(10) NOT NULL,
      c_comment VARCHAR(117) NOT NULL
    );
  */

  DECLARE_NOT_NULL_ENUM_COL(c_name_enum);
  DECLARE_NOT_NULL_ENUM_COL(c_address_enum);
  DECLARE_NOT_NULL_ENUM_COL(c_phone_enum);
  DECLARE_NOT_NULL_ENUM_COL(c_mktsegment_enum);
  DECLARE_NOT_NULL_ENUM_COL(c_comment_enum);

  std::ifstream fin(raw + "customer.tbl");
  int32_t tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, '|', 8);

    APPEND_NOT_NULL_ENUM(c_name_enum, data[1], tuple_idx);
    APPEND_NOT_NULL_ENUM(c_address_enum, data[2], tuple_idx);
    APPEND_NOT_NULL_ENUM(c_phone_enum, data[4], tuple_idx);
    APPEND_NOT_NULL_ENUM(c_mktsegment_enum, data[6], tuple_idx);
    APPEND_NOT_NULL_ENUM(c_comment_enum, data[7], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NOT_NULL_ENUM(c_name_enum, dest, "c_name_enum");
  SERIALIZE_NOT_NULL_ENUM(c_address_enum, dest, "c_address_enum");
  SERIALIZE_NOT_NULL_ENUM(c_phone_enum, dest, "c_phone_enum");
  SERIALIZE_NOT_NULL_ENUM(c_mktsegment_enum, dest, "c_mktsegment_enum");
  SERIALIZE_NOT_NULL_ENUM(c_comment_enum, dest, "c_comment_enum");
  Print("customer complete");
}

void Partsupp() {
  /*
    CREATE TABLE partsupp (
      ps_partkey INTEGER NOT NULL,
      ps_suppkey INTEGER NOT NULL,
      ps_availqty INTEGER NOT NULL,
      ps_supplycost NUMERIC NOT NULL,
      ps_comment VARCHAR(199) NOT NULL
    );
  */

  DECLARE_NOT_NULL_ENUM_COL(ps_comment_enum);

  std::ifstream fin(raw + "partsupp.tbl");
  int32_t tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, '|', 5);

    APPEND_NOT_NULL_ENUM(ps_comment_enum, data[4], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NOT_NULL_ENUM(ps_comment_enum, dest, "ps_comment_enum");
  Print("partsupp complete");
}

void Part() {
  /*
    CREATE TABLE part (
      p_partkey INTEGER NOT NULL,
      p_name VARCHAR(55) NOT NULL,
      p_mfgr CHAR(25) NOT NULL,
      p_brand CHAR(10) NOT NULL,
      p_type VARCHAR(25) NOT NULL,
      p_size INTEGER NOT NULL,
      p_container CHAR(10) NOT NULL,
      p_retailprice NUMERIC NOT NULL,
      p_comment VARCHAR(23) NOT NULL
    );
  */

  DECLARE_NOT_NULL_ENUM_COL(p_name_enum);
  DECLARE_NOT_NULL_ENUM_COL(p_mfgr_enum);
  DECLARE_NOT_NULL_ENUM_COL(p_brand_enum);
  DECLARE_NOT_NULL_ENUM_COL(p_type_enum);
  DECLARE_NOT_NULL_ENUM_COL(p_container_enum);
  DECLARE_NOT_NULL_ENUM_COL(p_comment_enum);

  std::ifstream fin(raw + "part.tbl");
  int32_t tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, '|', 9);

    APPEND_NOT_NULL_ENUM(p_name_enum, data[1], tuple_idx);
    APPEND_NOT_NULL_ENUM(p_mfgr_enum, data[2], tuple_idx);
    APPEND_NOT_NULL_ENUM(p_brand_enum, data[3], tuple_idx);
    APPEND_NOT_NULL_ENUM(p_type_enum, data[4], tuple_idx);
    APPEND_NOT_NULL_ENUM(p_container_enum, data[6], tuple_idx);
    APPEND_NOT_NULL_ENUM(p_comment_enum, data[8], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NOT_NULL_ENUM(p_name_enum, dest, "p_name_enum");
  SERIALIZE_NOT_NULL_ENUM(p_mfgr_enum, dest, "p_mfgr_enum");
  SERIALIZE_NOT_NULL_ENUM(p_brand_enum, dest, "p_brand_enum");
  SERIALIZE_NOT_NULL_ENUM(p_type_enum, dest, "p_type_enum");
  SERIALIZE_NOT_NULL_ENUM(p_container_enum, dest, "p_container_enum");
  SERIALIZE_NOT_NULL_ENUM(p_comment_enum, dest, "p_comment_enum");
  Print("part complete");
}

void Supplier() {
  /*
    CREATE TABLE supplier (
      s_suppkey  INTEGER NOT NULL,
      s_name CHAR(25) NOT NULL,
      s_address VARCHAR(40) NOT NULL,
      s_nationkey INTEGER NOT NULL,
      s_phone CHAR(15) NOT NULL,
      s_acctbal NUMERIC NOT NULL,
      s_comment VARCHAR(101) NOT NULL
    );
  */

  DECLARE_NOT_NULL_ENUM_COL(s_name_enum);
  DECLARE_NOT_NULL_ENUM_COL(s_address_enum);
  DECLARE_NOT_NULL_ENUM_COL(s_phone_enum);
  DECLARE_NOT_NULL_ENUM_COL(s_comment_enum);

  std::ifstream fin(raw + "supplier.tbl");
  int32_t tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, '|', 7);

    APPEND_NOT_NULL_ENUM(s_name_enum, data[1], tuple_idx);
    APPEND_NOT_NULL_ENUM(s_address_enum, data[2], tuple_idx);
    APPEND_NOT_NULL_ENUM(s_phone_enum, data[4], tuple_idx);
    APPEND_NOT_NULL_ENUM(s_comment_enum, data[6], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NOT_NULL_ENUM(s_name_enum, dest, "s_name_enum");
  SERIALIZE_NOT_NULL_ENUM(s_address_enum, dest, "s_address_enum");
  SERIALIZE_NOT_NULL_ENUM(s_phone_enum, dest, "s_phone_enum");
  SERIALIZE_NOT_NULL_ENUM(s_comment_enum, dest, "s_comment_enum");
  Print("supplier complete");
}

int main() {
  for (auto f :
       {Supplier, Part, Partsupp, Customer, Orders, Nation, Region, Lineitem}) {
    f();
  }
}
