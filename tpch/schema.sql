CREATE TABLE supplier (
    s_suppkey  INTEGER NOT NULL,
    s_name TEXT NOT NULL,
    s_address TEXT NOT NULL,
    s_nationkey INTEGER NOT NULL,
    s_phone TEXT NOT NULL,
    s_acctbal DOUBLE PRECISION NOT NULL,
    s_comment TEXT NOT NULL
);

CREATE TABLE part (
    p_partkey INTEGER NOT NULL,
    p_name TEXT NOT NULL,
    p_mfgr TEXT NOT NULL,
    p_brand TEXT NOT NULL,
    p_type TEXT NOT NULL,
    p_size INTEGER NOT NULL,
    p_container TEXT NOT NULL,
    p_retailprice DOUBLE PRECISION NOT NULL,
    p_comment TEXT NOT NULL
);

CREATE TABLE partsupp (
    ps_partkey INTEGER NOT NULL,
    ps_suppkey INTEGER NOT NULL,
    ps_availqty INTEGER NOT NULL,
    ps_supplycost DOUBLE PRECISION NOT NULL,
    ps_comment TEXT NOT NULL
);

CREATE TABLE customer (
    c_custkey INTEGER NOT NULL,
    c_name TEXT NOT NULL,
    c_address TEXT NOT NULL,
    c_nationkey INTEGER NOT NULL,
    c_phone TEXT NOT NULL,
    c_acctbal DOUBLE PRECISION NOT NULL,
    c_mktsegment TEXT NOT NULL,
    c_comment TEXT NOT NULL
);

CREATE TABLE orders (
    o_orderkey INTEGER NOT NULL,
    o_custkey INTEGER NOT NULL,
    o_orderstatus TEXT NOT NULL,
    o_totalprice DOUBLE PRECISION NOT NULL,
    o_orderdate TIMESTAMP NOT NULL,
    o_orderpriority TEXT NOT NULL,
    o_clerk TEXT NOT NULL,
    o_shippriority INTEGER NOT NULL,
    o_comment TEXT NOT NULL
);

CREATE TABLE lineitem (
    l_orderkey INTEGER NOT NULL,
    l_partkey INTEGER NOT NULL,
    l_suppkey INTEGER NOT NULL,
    l_linenumber INTEGER NOT NULL,
    l_quantity DOUBLE PRECISION NOT NULL,
    l_extendedprice DOUBLE PRECISION NOT NULL,
    l_discount DOUBLE PRECISION NOT NULL,
    l_tax DOUBLE PRECISION NOT NULL,
    l_returnflag TEXT NOT NULL,
    l_linestatus TEXT NOT NULL,
    l_shipdate TIMESTAMP NOT NULL,
    l_commitdate TIMESTAMP NOT NULL,
    l_receiptdate TIMESTAMP NOT NULL,
    l_shipinstruct TEXT NOT NULL,
    l_shipmode TEXT NOT NULL,
    l_comment TEXT NOT NULL
);

CREATE TABLE nation (
    n_nationkey INTEGER NOT NULL,
    n_name TEXT NOT NULL,
    n_regionkey INTEGER NOT NULL,
    n_comment TEXT NOT NULL
);

CREATE TABLE region (
    r_regionkey INTEGER NOT NULL,
    r_name TEXT NOT NULL,
    r_comment TEXT NOT NULL
);