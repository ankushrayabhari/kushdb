\copy customer FROM 'tpch/raw/customer.tbl' DELIMITER '|' CSV
\copy lineitem FROM 'tpch/raw/lineitem.tbl' DELIMITER '|' CSV
\copy nation FROM 'tpch/raw/nation.tbl' DELIMITER '|' CSV
\copy orders FROM 'tpch/raw/orders.tbl' DELIMITER '|' CSV
\copy part FROM 'tpch/raw/part.tbl' DELIMITER '|' CSV
\copy partsupp FROM 'tpch/raw/partsupp.tbl' DELIMITER '|' CSV
\copy region FROM 'tpch/raw/region.tbl' DELIMITER '|' CSV
\copy supplier FROM 'tpch/raw/supplier.tbl' DELIMITER '|' CSV