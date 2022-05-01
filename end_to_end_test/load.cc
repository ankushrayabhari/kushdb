
#include "util/load.h"

#include <cstdint>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>

using namespace kush::util;

const std::string dest("end_to_end_test/data/");
const std::string raw("end_to_end_test/raw/");

void People() {
  /*
    CREATE TABLE people (
      id INTEGER,
      name TEXT,
    );
  */

  DECLARE_NULL_COL(int32_t, id);
  DECLARE_NULL_COL(std::string, name);
  DECLARE_NULL_ENUM_COL(name_enum);

  std::ifstream fin(raw + "people.tbl");
  int32_t tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, '|', 2);

    APPEND_NULL(id, ParseInt32, data[0], tuple_idx);
    APPEND_NULL(name, ParseString, data[1], tuple_idx);
    APPEND_NULL_ENUM(name_enum, data[1], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NULL(int32_t, id, dest, "people_id");
  SERIALIZE_NULL(std::string, name, dest, "people_name");
  SERIALIZE_NULL_ENUM(name_enum, dest, "people_name_enum");
  std::cout << "people complete" << std::endl;
}

void Info() {
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

  DECLARE_NULL_COL(int32_t, id);
  DECLARE_NULL_COL(int8_t, cheated);
  DECLARE_NULL_COL(int32_t, date);
  DECLARE_NULL_COL(double, zscore);
  DECLARE_NULL_COL(int16_t, num1);
  DECLARE_NULL_COL(int64_t, num2);

  std::ifstream fin(raw + "info.tbl");
  int32_t tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, '|', 6);

    APPEND_NULL(id, ParseInt32, data[0], tuple_idx);
    APPEND_NULL(cheated, ParseBool, data[1], tuple_idx);
    APPEND_NULL(date, ParseDate, data[2], tuple_idx);
    APPEND_NULL(zscore, ParseDouble, data[3], tuple_idx);
    APPEND_NULL(num1, ParseInt16, data[4], tuple_idx);
    APPEND_NULL(num2, ParseInt64, data[5], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NULL(int32_t, id, dest, "info_id");
  SERIALIZE_NULL(int8_t, cheated, dest, "info_cheated");
  SERIALIZE_NULL(int32_t, date, dest, "info_date");
  SERIALIZE_NULL(double, zscore, dest, "info_zscore");
  SERIALIZE_NULL(int16_t, num1, dest, "info_num1");
  SERIALIZE_NULL(int64_t, num2, dest, "info_num2");
  std::cout << "info complete" << std::endl;
}

void Region() {
  /*
    CREATE TABLE region (
      r_regionkey INTEGER NOT NULL,
      r_name CHAR(25) NOT NULL,
      r_comment VARCHAR(152) NOT NULL
    );
  */

  DECLARE_NOT_NULL_COL(int32_t, r_regionkey);
  DECLARE_NOT_NULL_COL(std::string, r_name);
  DECLARE_NOT_NULL_ENUM_COL(r_name_enum);
  DECLARE_NOT_NULL_COL(std::string, r_comment);
  DECLARE_NOT_NULL_ENUM_COL(r_comment_enum);

  std::ifstream fin(raw + "region.tbl");
  int tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, '|', 3);

    APPEND_NOT_NULL(r_regionkey, ParseInt32, data[0], tuple_idx);
    APPEND_NOT_NULL(r_name, ParseString, data[1], tuple_idx);
    APPEND_NOT_NULL_ENUM(r_name_enum, data[1], tuple_idx);
    APPEND_NOT_NULL(r_comment, ParseString, data[2], tuple_idx);
    APPEND_NOT_NULL_ENUM(r_comment_enum, data[2], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NOT_NULL(int32_t, r_regionkey, dest, "r_regionkey");
  SERIALIZE_NOT_NULL(std::string, r_name, dest, "r_name");
  SERIALIZE_NOT_NULL_ENUM(r_name_enum, dest, "r_name_enum");
  SERIALIZE_NOT_NULL(std::string, r_comment, dest, "r_comment");
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

  DECLARE_NOT_NULL_COL(int32_t, n_nationkey);
  DECLARE_NOT_NULL_COL(std::string, n_name);
  DECLARE_NOT_NULL_ENUM_COL(n_name_enum);
  DECLARE_NOT_NULL_COL(int32_t, n_regionkey);
  DECLARE_NOT_NULL_COL(std::string, n_comment);
  DECLARE_NOT_NULL_ENUM_COL(n_comment_enum);

  std::ifstream fin(raw + "nation.tbl");
  int tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, '|', 4);

    APPEND_NOT_NULL(n_nationkey, ParseInt32, data[0], tuple_idx);
    APPEND_NOT_NULL(n_name, ParseString, data[1], tuple_idx);
    APPEND_NOT_NULL_ENUM(n_name_enum, data[1], tuple_idx);
    APPEND_NOT_NULL(n_regionkey, ParseInt32, data[2], tuple_idx);
    APPEND_NOT_NULL(n_comment, ParseString, data[3], tuple_idx);
    APPEND_NOT_NULL_ENUM(n_comment_enum, data[3], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NOT_NULL(int32_t, n_nationkey, dest, "n_nationkey");
  SERIALIZE_NOT_NULL(std::string, n_name, dest, "n_name");
  SERIALIZE_NOT_NULL_ENUM(n_name_enum, dest, "n_name_enum");
  SERIALIZE_NOT_NULL(int32_t, n_regionkey, dest, "n_regionkey");
  SERIALIZE_NOT_NULL(std::string, n_comment, dest, "n_comment");
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

  DECLARE_NOT_NULL_COL(int32_t, l_orderkey);
  DECLARE_NOT_NULL_COL(int32_t, l_partkey);
  DECLARE_NOT_NULL_COL(int32_t, l_suppkey);
  DECLARE_NOT_NULL_COL(int32_t, l_linenumber);
  DECLARE_NOT_NULL_COL(double, l_quantity);
  DECLARE_NOT_NULL_COL(double, l_extendedprice);
  DECLARE_NOT_NULL_COL(double, l_discount);
  DECLARE_NOT_NULL_COL(double, l_tax);
  DECLARE_NOT_NULL_COL(std::string, l_returnflag);
  DECLARE_NOT_NULL_ENUM_COL(l_returnflag_enum);
  DECLARE_NOT_NULL_COL(std::string, l_linestatus);
  DECLARE_NOT_NULL_ENUM_COL(l_linestatus_enum);
  DECLARE_NOT_NULL_COL(int32_t, l_shipdate);
  DECLARE_NOT_NULL_COL(int32_t, l_commitdate);
  DECLARE_NOT_NULL_COL(int32_t, l_receiptdate);
  DECLARE_NOT_NULL_COL(std::string, l_shipinstruct);
  DECLARE_NOT_NULL_ENUM_COL(l_shipinstruct_enum);
  DECLARE_NOT_NULL_COL(std::string, l_shipmode);
  DECLARE_NOT_NULL_ENUM_COL(l_shipmode_enum);
  DECLARE_NOT_NULL_COL(std::string, l_comment);
  DECLARE_NOT_NULL_ENUM_COL(l_comment_enum);

  std::ifstream fin(raw + "lineitem.tbl");
  int32_t tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, '|', 16);

    APPEND_NOT_NULL(l_orderkey, ParseInt32, data[0], tuple_idx);
    APPEND_NOT_NULL(l_partkey, ParseInt32, data[1], tuple_idx);
    APPEND_NOT_NULL(l_suppkey, ParseInt32, data[2], tuple_idx);
    APPEND_NOT_NULL(l_linenumber, ParseInt32, data[3], tuple_idx);
    APPEND_NOT_NULL(l_quantity, ParseDouble, data[4], tuple_idx);
    APPEND_NOT_NULL(l_extendedprice, ParseDouble, data[5], tuple_idx);
    APPEND_NOT_NULL(l_discount, ParseDouble, data[6], tuple_idx);
    APPEND_NOT_NULL(l_tax, ParseDouble, data[7], tuple_idx);
    APPEND_NOT_NULL(l_returnflag, ParseString, data[8], tuple_idx);
    APPEND_NOT_NULL_ENUM(l_returnflag_enum, data[8], tuple_idx);
    APPEND_NOT_NULL(l_linestatus, ParseString, data[9], tuple_idx);
    APPEND_NOT_NULL_ENUM(l_linestatus_enum, data[9], tuple_idx);
    APPEND_NOT_NULL(l_shipdate, ParseDate, data[10], tuple_idx);
    APPEND_NOT_NULL(l_commitdate, ParseDate, data[11], tuple_idx);
    APPEND_NOT_NULL(l_receiptdate, ParseDate, data[12], tuple_idx);
    APPEND_NOT_NULL(l_shipinstruct, ParseString, data[13], tuple_idx);
    APPEND_NOT_NULL_ENUM(l_shipinstruct_enum, data[13], tuple_idx);
    APPEND_NOT_NULL(l_shipmode, ParseString, data[14], tuple_idx);
    APPEND_NOT_NULL_ENUM(l_shipmode_enum, data[14], tuple_idx);
    APPEND_NOT_NULL(l_comment, ParseString, data[15], tuple_idx);
    APPEND_NOT_NULL_ENUM(l_comment_enum, data[15], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NOT_NULL(int32_t, l_orderkey, dest, "l_orderkey");
  SERIALIZE_NOT_NULL(int32_t, l_partkey, dest, "l_partkey");
  SERIALIZE_NOT_NULL(int32_t, l_suppkey, dest, "l_suppkey");
  SERIALIZE_NOT_NULL(int32_t, l_linenumber, dest, "l_linenumber");
  SERIALIZE_NOT_NULL(double, l_quantity, dest, "l_quantity");
  SERIALIZE_NOT_NULL(double, l_extendedprice, dest, "l_extendedprice");
  SERIALIZE_NOT_NULL(double, l_discount, dest, "l_discount");
  SERIALIZE_NOT_NULL(double, l_tax, dest, "l_tax");
  SERIALIZE_NOT_NULL(std::string, l_returnflag, dest, "l_returnflag");
  SERIALIZE_NOT_NULL_ENUM(l_returnflag_enum, dest, "l_returnflag_enum");
  SERIALIZE_NOT_NULL(std::string, l_linestatus, dest, "l_linestatus");
  SERIALIZE_NOT_NULL_ENUM(l_linestatus_enum, dest, "l_linestatus_enum");
  SERIALIZE_NOT_NULL(int32_t, l_shipdate, dest, "l_shipdate");
  SERIALIZE_NOT_NULL(int32_t, l_commitdate, dest, "l_commitdate");
  SERIALIZE_NOT_NULL(int32_t, l_receiptdate, dest, "l_receiptdate");
  SERIALIZE_NOT_NULL(std::string, l_shipinstruct, dest, "l_shipinstruct");
  SERIALIZE_NOT_NULL_ENUM(l_shipinstruct_enum, dest, "l_shipinstruct_enum");
  SERIALIZE_NOT_NULL(std::string, l_shipmode, dest, "l_shipmode");
  SERIALIZE_NOT_NULL_ENUM(l_shipmode_enum, dest, "l_shipmode_enum");
  SERIALIZE_NOT_NULL(std::string, l_comment, dest, "l_comment");
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

  DECLARE_NOT_NULL_COL(int32_t, o_orderkey);
  DECLARE_NOT_NULL_COL(int32_t, o_custkey);
  DECLARE_NOT_NULL_COL(std::string, o_orderstatus);
  DECLARE_NOT_NULL_ENUM_COL(o_orderstatus_enum);
  DECLARE_NOT_NULL_COL(double, o_totalprice);
  DECLARE_NOT_NULL_COL(int32_t, o_orderdate);
  DECLARE_NOT_NULL_COL(std::string, o_orderpriority);
  DECLARE_NOT_NULL_ENUM_COL(o_orderpriority_enum);
  DECLARE_NOT_NULL_COL(std::string, o_clerk);
  DECLARE_NOT_NULL_ENUM_COL(o_clerk_enum);
  DECLARE_NOT_NULL_COL(int32_t, o_shippriority);
  DECLARE_NOT_NULL_COL(std::string, o_comment);
  DECLARE_NOT_NULL_ENUM_COL(o_comment_enum);

  std::ifstream fin(raw + "orders.tbl");
  int32_t tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, '|', 9);

    APPEND_NOT_NULL(o_orderkey, ParseInt32, data[0], tuple_idx);
    APPEND_NOT_NULL(o_custkey, ParseInt32, data[1], tuple_idx);
    APPEND_NOT_NULL(o_orderstatus, ParseString, data[2], tuple_idx);
    APPEND_NOT_NULL_ENUM(o_orderstatus_enum, data[2], tuple_idx);
    APPEND_NOT_NULL(o_totalprice, ParseDouble, data[3], tuple_idx);
    APPEND_NOT_NULL(o_orderdate, ParseDate, data[4], tuple_idx);
    APPEND_NOT_NULL(o_orderpriority, ParseString, data[5], tuple_idx);
    APPEND_NOT_NULL_ENUM(o_orderpriority_enum, data[5], tuple_idx);
    APPEND_NOT_NULL(o_clerk, ParseString, data[6], tuple_idx);
    APPEND_NOT_NULL_ENUM(o_clerk_enum, data[6], tuple_idx);
    APPEND_NOT_NULL(o_shippriority, ParseInt32, data[7], tuple_idx);
    APPEND_NOT_NULL(o_comment, ParseString, data[8], tuple_idx);
    APPEND_NOT_NULL_ENUM(o_comment_enum, data[8], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NOT_NULL(int32_t, o_orderkey, dest, "o_orderkey");
  SERIALIZE_NOT_NULL(int32_t, o_custkey, dest, "o_custkey");
  SERIALIZE_NOT_NULL(std::string, o_orderstatus, dest, "o_orderstatus");
  SERIALIZE_NOT_NULL_ENUM(o_orderstatus_enum, dest, "o_orderstatus_enum");
  SERIALIZE_NOT_NULL(double, o_totalprice, dest, "o_totalprice");
  SERIALIZE_NOT_NULL(int32_t, o_orderdate, dest, "o_orderdate");
  SERIALIZE_NOT_NULL(std::string, o_orderpriority, dest, "o_orderpriority");
  SERIALIZE_NOT_NULL_ENUM(o_orderpriority_enum, dest, "o_orderpriority_enum");
  SERIALIZE_NOT_NULL(std::string, o_clerk, dest, "o_clerk");
  SERIALIZE_NOT_NULL_ENUM(o_clerk_enum, dest, "o_clerk_enum");
  SERIALIZE_NOT_NULL(int32_t, o_shippriority, dest, "o_shippriority");
  SERIALIZE_NOT_NULL(std::string, o_comment, dest, "o_comment");
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

  DECLARE_NOT_NULL_COL(int32_t, c_custkey);
  DECLARE_NOT_NULL_COL(std::string, c_name);
  DECLARE_NOT_NULL_ENUM_COL(c_name_enum);
  DECLARE_NOT_NULL_COL(std::string, c_address);
  DECLARE_NOT_NULL_ENUM_COL(c_address_enum);
  DECLARE_NOT_NULL_COL(int32_t, c_nationkey);
  DECLARE_NOT_NULL_COL(std::string, c_phone);
  DECLARE_NOT_NULL_ENUM_COL(c_phone_enum);
  DECLARE_NOT_NULL_COL(double, c_acctbal);
  DECLARE_NOT_NULL_COL(std::string, c_mktsegment);
  DECLARE_NOT_NULL_ENUM_COL(c_mktsegment_enum);
  DECLARE_NOT_NULL_COL(std::string, c_comment);
  DECLARE_NOT_NULL_ENUM_COL(c_comment_enum);

  std::ifstream fin(raw + "customer.tbl");
  int32_t tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, '|', 8);

    APPEND_NOT_NULL(c_custkey, ParseInt32, data[0], tuple_idx);
    APPEND_NOT_NULL(c_name, ParseString, data[1], tuple_idx);
    APPEND_NOT_NULL_ENUM(c_name_enum, data[1], tuple_idx);
    APPEND_NOT_NULL(c_address, ParseString, data[2], tuple_idx);
    APPEND_NOT_NULL_ENUM(c_address_enum, data[2], tuple_idx);
    APPEND_NOT_NULL(c_nationkey, ParseInt32, data[3], tuple_idx);
    APPEND_NOT_NULL(c_phone, ParseString, data[4], tuple_idx);
    APPEND_NOT_NULL_ENUM(c_phone_enum, data[4], tuple_idx);
    APPEND_NOT_NULL(c_acctbal, ParseDouble, data[5], tuple_idx);
    APPEND_NOT_NULL(c_mktsegment, ParseString, data[6], tuple_idx);
    APPEND_NOT_NULL_ENUM(c_mktsegment_enum, data[6], tuple_idx);
    APPEND_NOT_NULL(c_comment, ParseString, data[7], tuple_idx);
    APPEND_NOT_NULL_ENUM(c_comment_enum, data[7], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NOT_NULL(int32_t, c_custkey, dest, "c_custkey");
  SERIALIZE_NOT_NULL(std::string, c_name, dest, "c_name");
  SERIALIZE_NOT_NULL_ENUM(c_name_enum, dest, "c_name_enum");
  SERIALIZE_NOT_NULL(std::string, c_address, dest, "c_address");
  SERIALIZE_NOT_NULL_ENUM(c_address_enum, dest, "c_address_enum");
  SERIALIZE_NOT_NULL(int32_t, c_nationkey, dest, "c_nationkey");
  SERIALIZE_NOT_NULL(std::string, c_phone, dest, "c_phone");
  SERIALIZE_NOT_NULL_ENUM(c_phone_enum, dest, "c_phone_enum");
  SERIALIZE_NOT_NULL(double, c_acctbal, dest, "c_acctbal");
  SERIALIZE_NOT_NULL(std::string, c_mktsegment, dest, "c_mktsegment");
  SERIALIZE_NOT_NULL_ENUM(c_mktsegment_enum, dest, "c_mktsegment_enum");
  SERIALIZE_NOT_NULL(std::string, c_comment, dest, "c_comment");
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

  DECLARE_NOT_NULL_COL(int32_t, ps_partkey);
  DECLARE_NOT_NULL_COL(int32_t, ps_suppkey);
  DECLARE_NOT_NULL_COL(int32_t, ps_availqty);
  DECLARE_NOT_NULL_COL(double, ps_supplycost);
  DECLARE_NOT_NULL_COL(std::string, ps_comment);
  DECLARE_NOT_NULL_ENUM_COL(ps_comment_enum);

  std::ifstream fin(raw + "partsupp.tbl");
  int32_t tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, '|', 5);

    APPEND_NOT_NULL(ps_partkey, ParseInt32, data[0], tuple_idx);
    APPEND_NOT_NULL(ps_suppkey, ParseInt32, data[1], tuple_idx);
    APPEND_NOT_NULL(ps_availqty, ParseInt32, data[2], tuple_idx);
    APPEND_NOT_NULL(ps_supplycost, ParseDouble, data[3], tuple_idx);
    APPEND_NOT_NULL(ps_comment, ParseString, data[4], tuple_idx);
    APPEND_NOT_NULL_ENUM(ps_comment_enum, data[4], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NOT_NULL(int32_t, ps_partkey, dest, "ps_partkey");
  SERIALIZE_NOT_NULL(int32_t, ps_suppkey, dest, "ps_suppkey");
  SERIALIZE_NOT_NULL(int32_t, ps_availqty, dest, "ps_availqty");
  SERIALIZE_NOT_NULL(double, ps_supplycost, dest, "ps_supplycost");
  SERIALIZE_NOT_NULL(std::string, ps_comment, dest, "ps_comment");
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

  DECLARE_NOT_NULL_COL(int32_t, p_partkey);
  DECLARE_NOT_NULL_COL(std::string, p_name);
  DECLARE_NOT_NULL_ENUM_COL(p_name_enum);
  DECLARE_NOT_NULL_COL(std::string, p_mfgr);
  DECLARE_NOT_NULL_ENUM_COL(p_mfgr_enum);
  DECLARE_NOT_NULL_COL(std::string, p_brand);
  DECLARE_NOT_NULL_ENUM_COL(p_brand_enum);
  DECLARE_NOT_NULL_COL(std::string, p_type);
  DECLARE_NOT_NULL_ENUM_COL(p_type_enum);
  DECLARE_NOT_NULL_COL(int32_t, p_size);
  DECLARE_NOT_NULL_COL(std::string, p_container);
  DECLARE_NOT_NULL_ENUM_COL(p_container_enum);
  DECLARE_NOT_NULL_COL(double, p_retailprice);
  DECLARE_NOT_NULL_COL(std::string, p_comment);
  DECLARE_NOT_NULL_ENUM_COL(p_comment_enum);

  std::ifstream fin(raw + "part.tbl");
  int32_t tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, '|', 9);

    APPEND_NOT_NULL(p_partkey, ParseInt32, data[0], tuple_idx);
    APPEND_NOT_NULL(p_name, ParseString, data[1], tuple_idx);
    APPEND_NOT_NULL_ENUM(p_name_enum, data[1], tuple_idx);
    APPEND_NOT_NULL(p_mfgr, ParseString, data[2], tuple_idx);
    APPEND_NOT_NULL_ENUM(p_mfgr_enum, data[2], tuple_idx);
    APPEND_NOT_NULL(p_brand, ParseString, data[3], tuple_idx);
    APPEND_NOT_NULL_ENUM(p_brand_enum, data[3], tuple_idx);
    APPEND_NOT_NULL(p_type, ParseString, data[4], tuple_idx);
    APPEND_NOT_NULL_ENUM(p_type_enum, data[4], tuple_idx);
    APPEND_NOT_NULL(p_size, ParseInt32, data[5], tuple_idx);
    APPEND_NOT_NULL(p_container, ParseString, data[6], tuple_idx);
    APPEND_NOT_NULL_ENUM(p_container_enum, data[6], tuple_idx);
    APPEND_NOT_NULL(p_retailprice, ParseDouble, data[7], tuple_idx);
    APPEND_NOT_NULL(p_comment, ParseString, data[8], tuple_idx);
    APPEND_NOT_NULL_ENUM(p_comment_enum, data[8], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NOT_NULL(int32_t, p_partkey, dest, "p_partkey");
  SERIALIZE_NOT_NULL(std::string, p_name, dest, "p_name");
  SERIALIZE_NOT_NULL_ENUM(p_name_enum, dest, "p_name_enum");
  SERIALIZE_NOT_NULL(std::string, p_mfgr, dest, "p_mfgr");
  SERIALIZE_NOT_NULL_ENUM(p_mfgr_enum, dest, "p_mfgr_enum");
  SERIALIZE_NOT_NULL(std::string, p_brand, dest, "p_brand");
  SERIALIZE_NOT_NULL_ENUM(p_brand_enum, dest, "p_brand_enum");
  SERIALIZE_NOT_NULL(std::string, p_type, dest, "p_type");
  SERIALIZE_NOT_NULL_ENUM(p_type_enum, dest, "p_type_enum");
  SERIALIZE_NOT_NULL(int32_t, p_size, dest, "p_size");
  SERIALIZE_NOT_NULL(std::string, p_container, dest, "p_container");
  SERIALIZE_NOT_NULL_ENUM(p_container_enum, dest, "p_container_enum");
  SERIALIZE_NOT_NULL(double, p_retailprice, dest, "p_retailprice");
  SERIALIZE_NOT_NULL(std::string, p_comment, dest, "p_comment");
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

  DECLARE_NOT_NULL_COL(int32_t, s_suppkey);
  DECLARE_NOT_NULL_COL(std::string, s_name);
  DECLARE_NOT_NULL_ENUM_COL(s_name_enum);
  DECLARE_NOT_NULL_COL(std::string, s_address);
  DECLARE_NOT_NULL_ENUM_COL(s_address_enum);
  DECLARE_NOT_NULL_COL(int32_t, s_nationkey);
  DECLARE_NOT_NULL_COL(std::string, s_phone);
  DECLARE_NOT_NULL_ENUM_COL(s_phone_enum);
  DECLARE_NOT_NULL_COL(double, s_acctbal);
  DECLARE_NOT_NULL_COL(std::string, s_comment);
  DECLARE_NOT_NULL_ENUM_COL(s_comment_enum);

  std::ifstream fin(raw + "supplier.tbl");
  int32_t tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, '|', 7);

    APPEND_NOT_NULL(s_suppkey, ParseInt32, data[0], tuple_idx);
    APPEND_NOT_NULL(s_name, ParseString, data[1], tuple_idx);
    APPEND_NOT_NULL_ENUM(s_name_enum, data[1], tuple_idx);
    APPEND_NOT_NULL(s_address, ParseString, data[2], tuple_idx);
    APPEND_NOT_NULL_ENUM(s_address_enum, data[2], tuple_idx);
    APPEND_NOT_NULL(s_nationkey, ParseInt32, data[3], tuple_idx);
    APPEND_NOT_NULL(s_phone, ParseString, data[4], tuple_idx);
    APPEND_NOT_NULL_ENUM(s_phone_enum, data[4], tuple_idx);
    APPEND_NOT_NULL(s_acctbal, ParseDouble, data[5], tuple_idx);
    APPEND_NOT_NULL(s_comment, ParseString, data[6], tuple_idx);
    APPEND_NOT_NULL_ENUM(s_comment_enum, data[6], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NOT_NULL(int32_t, s_suppkey, dest, "s_suppkey");
  SERIALIZE_NOT_NULL(std::string, s_name, dest, "s_name");
  SERIALIZE_NOT_NULL_ENUM(s_name_enum, dest, "s_name_enum");
  SERIALIZE_NOT_NULL(std::string, s_address, dest, "s_address");
  SERIALIZE_NOT_NULL_ENUM(s_address_enum, dest, "s_address_enum");
  SERIALIZE_NOT_NULL(int32_t, s_nationkey, dest, "s_nationkey");
  SERIALIZE_NOT_NULL(std::string, s_phone, dest, "s_phone");
  SERIALIZE_NOT_NULL_ENUM(s_phone_enum, dest, "s_phone_enum");
  SERIALIZE_NOT_NULL(double, s_acctbal, dest, "s_acctbal");
  SERIALIZE_NOT_NULL(std::string, s_comment, dest, "s_comment");
  SERIALIZE_NOT_NULL_ENUM(s_comment_enum, dest, "s_comment_enum");
  Print("supplier complete");
}

int main() {
  std::vector<std::function<void(void)>> loads{
      People,   Info,   Supplier, Part,   Partsupp,
      Customer, Orders, Lineitem, Nation, Region};
  for (auto& f : loads) {
    f();
  }
}
