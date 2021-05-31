#include <algorithm>
#include <execution>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <sstream>
#include <string>

#include "absl/time/civil_time.h"
#include "absl/time/time.h"

#include "runtime/column_data_builder.h"

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

  std::ifstream fin("tpch/raw/region.tbl");
  for (std::string line; std::getline(fin, line);) {
    auto data = split(line, '|');

    r_regionkey.push_back(std::stoi(data[0]));
    r_name.push_back(data[1]);
    r_comment.push_back(data[2]);
  }

  kush::runtime::ColumnData::Serialize<int32_t>("tpch/data/r_regionkey.kdb",
                                                r_regionkey);
  kush::runtime::ColumnData::Serialize<std::string>("tpch/data/r_name.kdb",
                                                    r_name);
  kush::runtime::ColumnData::Serialize<std::string>("tpch/data/r_comment.kdb",
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

  std::ifstream fin("tpch/raw/nation.tbl");
  for (std::string line; std::getline(fin, line);) {
    auto data = split(line, '|');

    n_nationkey.push_back(std::stoi(data[0]));
    n_name.push_back(data[1]);
    n_regionkey.push_back(std::stoi(data[2]));
    n_comment.push_back(data[3]);
  }

  kush::runtime::ColumnData::Serialize<int32_t>("tpch/data/n_nationkey.kdb",
                                                n_nationkey);
  kush::runtime::ColumnData::Serialize<std::string>("tpch/data/n_name.kdb",
                                                    n_name);
  kush::runtime::ColumnData::Serialize<int32_t>("tpch/data/n_regionkey.kdb",
                                                n_regionkey);
  kush::runtime::ColumnData::Serialize<std::string>("tpch/data/n_comment.kdb",
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

  std::ifstream fin("tpch/raw/lineitem.tbl");
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

  kush::runtime::ColumnData::Serialize<int32_t>("tpch/data/l_orderkey.kdb",
                                                l_orderkey);
  kush::runtime::ColumnData::Serialize<int32_t>("tpch/data/l_partkey.kdb",
                                                l_partkey);
  kush::runtime::ColumnData::Serialize<int32_t>("tpch/data/l_suppkey.kdb",
                                                l_suppkey);
  kush::runtime::ColumnData::Serialize<int32_t>("tpch/data/l_linenumber.kdb",
                                                l_linenumber);
  kush::runtime::ColumnData::Serialize<double>("tpch/data/l_quantity.kdb",
                                               l_quantity);
  kush::runtime::ColumnData::Serialize<double>("tpch/data/l_extendedprice.kdb",
                                               l_extendedprice);
  kush::runtime::ColumnData::Serialize<double>("tpch/data/l_discount.kdb",
                                               l_discount);
  kush::runtime::ColumnData::Serialize<double>("tpch/data/l_tax.kdb", l_tax);
  kush::runtime::ColumnData::Serialize<std::string>(
      "tpch/data/l_returnflag.kdb", l_returnflag);
  kush::runtime::ColumnData::Serialize<std::string>(
      "tpch/data/l_linestatus.kdb", l_linestatus);
  kush::runtime::ColumnData::Serialize<int64_t>("tpch/data/l_shipdate.kdb",
                                                l_shipdate);
  kush::runtime::ColumnData::Serialize<int64_t>("tpch/data/l_commitdate.kdb",
                                                l_commitdate);
  kush::runtime::ColumnData::Serialize<int64_t>("tpch/data/l_receiptdate.kdb",
                                                l_receiptdate);
  kush::runtime::ColumnData::Serialize<std::string>(
      "tpch/data/l_shipinstruct.kdb", l_shipinstruct);
  kush::runtime::ColumnData::Serialize<std::string>("tpch/data/l_shipmode.kdb",
                                                    l_shipmode);
  kush::runtime::ColumnData::Serialize<std::string>("tpch/data/l_comment.kdb",
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

  std::ifstream fin("tpch/raw/orders.tbl");
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

  kush::runtime::ColumnData::Serialize<int32_t>("tpch/data/o_orderkey.kdb",
                                                o_orderkey);
  kush::runtime::ColumnData::Serialize<int32_t>("tpch/data/o_custkey.kdb",
                                                o_custkey);
  kush::runtime::ColumnData::Serialize<std::string>(
      "tpch/data/o_orderstatus.kdb", o_orderstatus);
  kush::runtime::ColumnData::Serialize<double>("tpch/data/o_totalprice.kdb",
                                               o_totalprice);
  kush::runtime::ColumnData::Serialize<int64_t>("tpch/data/o_orderdate.kdb",
                                                o_orderdate);
  kush::runtime::ColumnData::Serialize<std::string>(
      "tpch/data/o_orderpriority.kdb", o_orderpriority);
  kush::runtime::ColumnData::Serialize<std::string>("tpch/data/o_clerk.kdb",
                                                    o_clerk);
  kush::runtime::ColumnData::Serialize<int32_t>("tpch/data/o_shippriority.kdb",
                                                o_shippriority);
  kush::runtime::ColumnData::Serialize<std::string>("tpch/data/o_comment.kdb",
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

  std::ifstream fin("tpch/raw/customer.tbl");
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

  kush::runtime::ColumnData::Serialize<int32_t>("tpch/data/c_custkey.kdb",
                                                c_custkey);
  kush::runtime::ColumnData::Serialize<std::string>("tpch/data/c_name.kdb",
                                                    c_name);
  kush::runtime::ColumnData::Serialize<std::string>("tpch/data/c_address.kdb",
                                                    c_address);
  kush::runtime::ColumnData::Serialize<int32_t>("tpch/data/c_nationkey.kdb",
                                                c_nationkey);
  kush::runtime::ColumnData::Serialize<std::string>("tpch/data/c_phone.kdb",
                                                    c_phone);
  kush::runtime::ColumnData::Serialize<double>("tpch/data/c_acctbal.kdb",
                                               c_acctbal);
  kush::runtime::ColumnData::Serialize<std::string>(
      "tpch/data/c_mktsegment.kdb", c_mktsegment);
  kush::runtime::ColumnData::Serialize<std::string>("tpch/data/c_comment.kdb",
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

  std::ifstream fin("tpch/raw/partsupp.tbl");
  for (std::string line; std::getline(fin, line);) {
    auto data = split(line, '|');

    ps_partkey.push_back(std::stoi(data[0]));
    ps_suppkey.push_back(std::stoi(data[1]));
    ps_availqty.push_back(std::stoi(data[2]));
    ps_supplycost.push_back(std::stod(data[3]));
    ps_comment.push_back(data[4]);
  }

  kush::runtime::ColumnData::Serialize<int32_t>("tpch/data/ps_partkey.kdb",
                                                ps_partkey);
  kush::runtime::ColumnData::Serialize<int32_t>("tpch/data/ps_suppkey.kdb",
                                                ps_suppkey);
  kush::runtime::ColumnData::Serialize<int32_t>("tpch/data/ps_availqty.kdb",
                                                ps_availqty);
  kush::runtime::ColumnData::Serialize<double>("tpch/data/ps_supplycost.kdb",
                                               ps_supplycost);
  kush::runtime::ColumnData::Serialize<std::string>("tpch/data/ps_comment.kdb",
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

  std::ifstream fin("tpch/raw/part.tbl");
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

  kush::runtime::ColumnData::Serialize<int32_t>("tpch/data/p_partkey.kdb",
                                                p_partkey);
  kush::runtime::ColumnData::Serialize<std::string>("tpch/data/p_name.kdb",
                                                    p_name);
  kush::runtime::ColumnData::Serialize<std::string>("tpch/data/p_mfgr.kdb",
                                                    p_mfgr);
  kush::runtime::ColumnData::Serialize<std::string>("tpch/data/p_brand.kdb",
                                                    p_brand);
  kush::runtime::ColumnData::Serialize<std::string>("tpch/data/p_type.kdb",
                                                    p_type);
  kush::runtime::ColumnData::Serialize<int32_t>("tpch/data/p_size.kdb", p_size);
  kush::runtime::ColumnData::Serialize<std::string>("tpch/data/p_container.kdb",
                                                    p_container);
  kush::runtime::ColumnData::Serialize<double>("tpch/data/p_retailprice.kdb",
                                               p_retailprice);
  kush::runtime::ColumnData::Serialize<std::string>("tpch/data/p_comment.kdb",
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

  std::ifstream fin("tpch/raw/supplier.tbl");
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

  kush::runtime::ColumnData::Serialize<int32_t>("tpch/data/s_suppkey.kdb",
                                                s_suppkey);
  kush::runtime::ColumnData::Serialize<std::string>("tpch/data/s_name.kdb",
                                                    s_name);
  kush::runtime::ColumnData::Serialize<std::string>("tpch/data/s_address.kdb",
                                                    s_address);
  kush::runtime::ColumnData::Serialize<int32_t>("tpch/data/s_nationkey.kdb",
                                                s_nationkey);
  kush::runtime::ColumnData::Serialize<std::string>("tpch/data/s_phone.kdb",
                                                    s_phone);
  kush::runtime::ColumnData::Serialize<double>("tpch/data/s_acctbal.kdb",
                                               s_acctbal);
  kush::runtime::ColumnData::Serialize<std::string>("tpch/data/s_comment.kdb",
                                                    s_comment);
}

int main() {
  std::vector<std::function<void(void)>> loads{
      Supplier, Part, Partsupp, Customer, Orders, Lineitem, Nation, Region};
  std::for_each(std::execution::par_unseq, loads.begin(), loads.end(),
                [](auto&& item) { item(); });
}
