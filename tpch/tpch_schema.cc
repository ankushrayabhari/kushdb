#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <sstream>
#include <string>

#include "absl/time/civil_time.h"
#include "absl/time/time.h"
#include "data/column_data.h"

std::vector<std::string> split(const std::string& s, char delim) {
  std::stringstream ss(s);
  std::string item;
  std::vector<std::string> elems;
  while (std::getline(ss, item, delim)) {
    elems.push_back(std::move(item));
  }
  return elems;
}

int64_t ParseDate(const std::string& s) {
  auto parts = split(s, '-');
  auto day = absl::CivilDay(std::stoi(parts[0]), std::stoi(parts[1]),
                            std::stoi(parts[2]));
  absl::TimeZone utc = absl::UTCTimeZone();
  absl::Time time = absl::FromCivil(day, utc);
  return absl::ToUnixMillis(time);
}

void Region() {
  /*
    CREATE TABLE region (
      r_regionkey INTEGER NOT NULL,
      r_name CHAR(25) NOT NULL,
      r_comment VARCHAR(152) NOT NULL
    );
  */

  std::vector<int32_t> r_regionkey;
  std::vector<std::string> r_name;
  std::vector<std::string> r_comment;

  std::ifstream fin("tpch-data/region.tbl");
  for (std::string line; std::getline(fin, line);) {
    auto data = split(line, '|');

    r_regionkey.push_back(std::stoi(data[0]));
    r_name.push_back(data[1]);
    r_comment.push_back(data[2]);
  }

  kush::data::ColumnData<int32_t>::Serialize("r_regionkey.kdb", r_regionkey);
  kush::data::ColumnData<std::string_view>::Serialize("r_name.kdb", r_name);
  kush::data::ColumnData<std::string_view>::Serialize("r_comment.kdb",
                                                      r_comment);
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

  std::vector<int32_t> n_nationkey;
  std::vector<std::string> n_name;
  std::vector<int32_t> n_regionkey;
  std::vector<std::string> n_comment;

  std::ifstream fin("tpch-data/nation.tbl");
  for (std::string line; std::getline(fin, line);) {
    auto data = split(line, '|');

    n_nationkey.push_back(std::stoi(data[0]));
    n_name.push_back(data[1]);
    n_regionkey.push_back(std::stoi(data[2]));
    n_comment.push_back(data[3]);
  }

  kush::data::ColumnData<int32_t>::Serialize("n_nationkey.kdb", n_nationkey);
  kush::data::ColumnData<std::string_view>::Serialize("n_name.kdb", n_name);
  kush::data::ColumnData<int32_t>::Serialize("n_regionkey.kdb", n_regionkey);
  kush::data::ColumnData<std::string_view>::Serialize("n_comment.kdb",
                                                      n_comment);
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

  std::vector<int32_t> l_orderkey;
  std::vector<int32_t> l_partkey;
  std::vector<int32_t> l_suppkey;
  std::vector<int32_t> l_linenumber;
  std::vector<double> l_quantity;
  std::vector<double> l_extendedprice;
  std::vector<double> l_discount;
  std::vector<double> l_tax;
  std::vector<std::string> l_returnflag;
  std::vector<std::string> l_linestatus;
  std::vector<int64_t> l_shipdate;
  std::vector<int64_t> l_commitdate;
  std::vector<int64_t> l_receiptdate;
  std::vector<std::string> l_shipinstruct;
  std::vector<std::string> l_shipmode;
  std::vector<std::string> l_comment;

  std::ifstream fin("tpch-data/lineitem.tbl");
  for (std::string line; std::getline(fin, line);) {
    auto data = split(line, '|');

    l_orderkey.push_back(std::stoi(data[0]));
    l_partkey.push_back(std::stoi(data[1]));
    l_suppkey.push_back(std::stoi(data[2]));
    l_linenumber.push_back(std::stoi(data[3]));
    l_quantity.push_back(std::stod(data[4]));
    l_extendedprice.push_back(std::stod(data[5]));
    l_discount.push_back(std::stod(data[6]));
    l_tax.push_back(std::stod(data[7]));
    l_returnflag.push_back(data[8]);
    l_linestatus.push_back(data[9]);
    l_shipdate.push_back(ParseDate(data[10]));
    l_commitdate.push_back(ParseDate(data[11]));
    l_receiptdate.push_back(ParseDate(data[12]));
    l_shipinstruct.push_back(data[13]);
    l_shipmode.push_back(data[14]);
    l_comment.push_back(data[15]);
  }

  kush::data::ColumnData<int32_t>::Serialize("l_orderkey.kdb", l_orderkey);
  kush::data::ColumnData<int32_t>::Serialize("l_partkey.kdb", l_partkey);
  kush::data::ColumnData<int32_t>::Serialize("l_suppkey.kdb", l_suppkey);
  kush::data::ColumnData<int32_t>::Serialize("l_linenumber.kdb", l_linenumber);
  kush::data::ColumnData<double>::Serialize("l_quantity.kdb", l_quantity);
  kush::data::ColumnData<double>::Serialize("l_extendedprice.kdb",
                                            l_extendedprice);
  kush::data::ColumnData<double>::Serialize("l_discount.kdb", l_discount);
  kush::data::ColumnData<double>::Serialize("l_tax.kdb", l_tax);
  kush::data::ColumnData<std::string_view>::Serialize("l_returnflag.kdb",
                                                      l_returnflag);
  kush::data::ColumnData<std::string_view>::Serialize("l_linestatus.kdb",
                                                      l_linestatus);
  kush::data::ColumnData<int64_t>::Serialize("l_shipdate.kdb", l_shipdate);
  kush::data::ColumnData<int64_t>::Serialize("l_commitdate.kdb", l_commitdate);
  kush::data::ColumnData<int64_t>::Serialize("l_receiptdate.kdb",
                                             l_receiptdate);
  kush::data::ColumnData<std::string_view>::Serialize("l_shipinstruct.kdb",
                                                      l_shipinstruct);
  kush::data::ColumnData<std::string_view>::Serialize("l_shipmode.kdb",
                                                      l_shipmode);
  kush::data::ColumnData<std::string_view>::Serialize("l_comment.kdb",
                                                      l_comment);
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

  std::vector<int32_t> o_orderkey;
  std::vector<int32_t> o_custkey;
  std::vector<std::string> o_orderstatus;
  std::vector<double> o_totalprice;
  std::vector<int64_t> o_orderdate;
  std::vector<std::string> o_orderpriority;
  std::vector<std::string> o_clerk;
  std::vector<int32_t> o_shippriority;
  std::vector<std::string> o_comment;

  std::ifstream fin("tpch-data/orders.tbl");
  for (std::string line; std::getline(fin, line);) {
    auto data = split(line, '|');

    o_orderkey.push_back(std::stoi(data[0]));
    o_custkey.push_back(std::stoi(data[1]));
    o_orderstatus.push_back(data[2]);
    o_totalprice.push_back(std::stod(data[3]));
    o_orderdate.push_back(ParseDate(data[4]));
    o_orderpriority.push_back(data[5]);
    o_clerk.push_back(data[6]);
    o_shippriority.push_back(std::stoi(data[7]));
    o_comment.push_back(data[8]);
  }

  kush::data::ColumnData<int32_t>::Serialize("o_orderkey.kdb", o_orderkey);
  kush::data::ColumnData<int32_t>::Serialize("o_custkey.kdb", o_custkey);
  kush::data::ColumnData<std::string_view>::Serialize("o_orderstatus.kdb",
                                                      o_orderstatus);
  kush::data::ColumnData<double>::Serialize("o_totalprice.kdb", o_totalprice);
  kush::data::ColumnData<int64_t>::Serialize("o_orderdate.kdb", o_orderdate);
  kush::data::ColumnData<std::string_view>::Serialize("o_orderpriority.kdb",
                                                      o_orderpriority);
  kush::data::ColumnData<std::string_view>::Serialize("o_clerk.kdb", o_clerk);
  kush::data::ColumnData<int32_t>::Serialize("o_shippriority.kdb",
                                             o_shippriority);
  kush::data::ColumnData<std::string_view>::Serialize("o_comment.kdb",
                                                      o_comment);
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

  std::vector<int32_t> c_custkey;
  std::vector<std::string> c_name;
  std::vector<std::string> c_address;
  std::vector<int32_t> c_nationkey;
  std::vector<std::string> c_phone;
  std::vector<double> c_acctbal;
  std::vector<std::string> c_mktsegment;
  std::vector<std::string> c_comment;

  std::ifstream fin("tpch-data/customer.tbl");
  for (std::string line; std::getline(fin, line);) {
    auto data = split(line, '|');

    c_custkey.push_back(std::stoi(data[0]));
    c_name.push_back(data[1]);
    c_address.push_back(data[2]);
    c_nationkey.push_back(std::stoi(data[3]));
    c_phone.push_back(data[4]);
    c_acctbal.push_back(std::stod(data[5]));
    c_mktsegment.push_back(data[6]);
    c_comment.push_back(data[7]);
  }

  kush::data::ColumnData<int32_t>::Serialize("c_custkey.kdb", c_custkey);
  kush::data::ColumnData<std::string_view>::Serialize("c_name.kdb", c_name);
  kush::data::ColumnData<std::string_view>::Serialize("c_address.kdb",
                                                      c_address);
  kush::data::ColumnData<int32_t>::Serialize("c_nationkey.kdb", c_nationkey);
  kush::data::ColumnData<std::string_view>::Serialize("c_phone.kdb", c_phone);
  kush::data::ColumnData<double>::Serialize("c_acctbal.kdb", c_acctbal);
  kush::data::ColumnData<std::string_view>::Serialize("c_mktsegment.kdb",
                                                      c_mktsegment);
  kush::data::ColumnData<std::string_view>::Serialize("c_comment.kdb",
                                                      c_comment);
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

  std::vector<int32_t> ps_partkey;
  std::vector<int32_t> ps_suppkey;
  std::vector<int32_t> ps_availqty;
  std::vector<double> ps_supplycost;
  std::vector<std::string> ps_comment;

  std::ifstream fin("tpch-data/partsupp.tbl");
  for (std::string line; std::getline(fin, line);) {
    auto data = split(line, '|');

    ps_partkey.push_back(std::stoi(data[0]));
    ps_suppkey.push_back(std::stoi(data[1]));
    ps_availqty.push_back(std::stoi(data[2]));
    ps_supplycost.push_back(std::stod(data[3]));
    ps_comment.push_back(data[4]);
  }

  kush::data::ColumnData<int32_t>::Serialize("ps_partkey.kdb", ps_partkey);
  kush::data::ColumnData<int32_t>::Serialize("ps_suppkey.kdb", ps_suppkey);
  kush::data::ColumnData<int32_t>::Serialize("ps_availqty.kdb", ps_availqty);
  kush::data::ColumnData<double>::Serialize("ps_supplycost.kdb", ps_supplycost);
  kush::data::ColumnData<std::string_view>::Serialize("ps_comment.kdb",
                                                      ps_comment);
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

  std::vector<int32_t> p_partkey;
  std::vector<std::string> p_name;
  std::vector<std::string> p_mfgr;
  std::vector<std::string> p_brand;
  std::vector<std::string> p_type;
  std::vector<int32_t> p_size;
  std::vector<std::string> p_container;
  std::vector<double> p_retailprice;
  std::vector<std::string> p_comment;

  std::ifstream fin("tpch-data/part.tbl");
  for (std::string line; std::getline(fin, line);) {
    auto data = split(line, '|');

    p_partkey.push_back(std::stoi(data[0]));
    p_name.push_back(data[1]);
    p_mfgr.push_back(data[2]);
    p_brand.push_back(data[3]);
    p_type.push_back(data[4]);
    p_size.push_back(std::stoi(data[5]));
    p_container.push_back(data[6]);
    p_retailprice.push_back(std::stod(data[7]));
    p_comment.push_back(data[8]);
  }

  kush::data::ColumnData<int32_t>::Serialize("p_partkey.kdb", p_partkey);
  kush::data::ColumnData<std::string_view>::Serialize("p_name.kdb", p_name);
  kush::data::ColumnData<std::string_view>::Serialize("p_mfgr.kdb", p_mfgr);
  kush::data::ColumnData<std::string_view>::Serialize("p_brand.kdb", p_brand);
  kush::data::ColumnData<std::string_view>::Serialize("p_type.kdb", p_type);
  kush::data::ColumnData<int32_t>::Serialize("p_size.kdb", p_size);
  kush::data::ColumnData<std::string_view>::Serialize("p_container.kdb",
                                                      p_container);
  kush::data::ColumnData<double>::Serialize("p_retailprice.kdb", p_retailprice);
  kush::data::ColumnData<std::string_view>::Serialize("p_comment.kdb",
                                                      p_comment);
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

  std::vector<int32_t> s_suppkey;
  std::vector<std::string> s_name;
  std::vector<std::string> s_address;
  std::vector<int32_t> s_nationkey;
  std::vector<std::string> s_phone;
  std::vector<double> s_acctbal;
  std::vector<std::string> s_comment;

  std::ifstream fin("tpch-data/supplier.tbl");
  for (std::string line; std::getline(fin, line);) {
    auto data = split(line, '|');

    s_suppkey.push_back(std::stoi(data[0]));
    s_name.push_back(data[1]);
    s_address.push_back(data[2]);
    s_nationkey.push_back(std::stoi(data[3]));
    s_phone.push_back(data[4]);
    s_acctbal.push_back(std::stod(data[5]));
    s_comment.push_back(data[6]);
  }

  kush::data::ColumnData<int32_t>::Serialize("s_suppkey.kdb", s_suppkey);
  kush::data::ColumnData<std::string_view>::Serialize("s_name.kdb", s_name);
  kush::data::ColumnData<std::string_view>::Serialize("s_address.kdb",
                                                      s_address);
  kush::data::ColumnData<int32_t>::Serialize("s_nationkey.kdb", s_nationkey);
  kush::data::ColumnData<std::string_view>::Serialize("s_phone.kdb", s_phone);
  kush::data::ColumnData<double>::Serialize("s_acctbal.kdb", s_acctbal);
  kush::data::ColumnData<std::string_view>::Serialize("s_comment.kdb",
                                                      s_comment);
}

int main() {
  Supplier();
  return 0;
}
