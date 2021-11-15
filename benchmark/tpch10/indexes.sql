CREATE INDEX nation_n_regionkey ON nation(n_regionkey);
CREATE INDEX supplier_s_nationkey ON supplier(s_nationkey);
CREATE INDEX customer_c_nationkey ON customer(c_nationkey);
CREATE INDEX orders_o_custkey ON orders(o_custkey);
CREATE INDEX lineitem_l_orderkey ON lineitem(l_orderkey);
CREATE INDEX lineitem_l_partkey_l_suppkey ON lineitem(l_partkey, l_suppkey);