\copy customer FROM '/home/ankushrayabhari/src/kushdb/tpch/raw/customer.tbl' DELIMITER '|' CSV
\copy lineitem FROM '/home/ankushrayabhari/src/kushdb/tpch/raw/lineitem.tbl' DELIMITER '|' CSV
\copy nation FROM '/home/ankushrayabhari/src/kushdb/tpch/raw/nation.tbl' DELIMITER '|' CSV
\copy orders FROM '/home/ankushrayabhari/src/kushdb/tpch/raw/orders.tbl' DELIMITER '|' CSV
\copy part FROM '/home/ankushrayabhari/src/kushdb/tpch/raw/part.tbl' DELIMITER '|' CSV
\copy partsupp FROM '/home/ankushrayabhari/src/kushdb/tpch/raw/partsupp.tbl' DELIMITER '|' CSV
\copy region FROM '/home/ankushrayabhari/src/kushdb/tpch/raw/region.tbl' DELIMITER '|' CSV
\copy supplier FROM '/home/ankushrayabhari/src/kushdb/tpch/raw/supplier.tbl' DELIMITER '|' CSV