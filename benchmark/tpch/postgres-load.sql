\copy customer FROM 'benchmark/tpch/raw/customer.tbl' DELIMITER '|' CSV
\copy lineitem FROM 'benchmark/tpch/raw/lineitem.tbl' DELIMITER '|' CSV
\copy nation FROM 'benchmark/tpch/raw/nation.tbl' DELIMITER '|' CSV
\copy orders FROM 'benchmark/tpch/raw/orders.tbl' DELIMITER '|' CSV
\copy part FROM 'benchmark/tpch/raw/part.tbl' DELIMITER '|' CSV
\copy partsupp FROM 'benchmark/tpch/raw/partsupp.tbl' DELIMITER '|' CSV
\copy region FROM 'benchmark/tpch/raw/region.tbl' DELIMITER '|' CSV
\copy supplier FROM 'benchmark/tpch/raw/supplier.tbl' DELIMITER '|' CSV