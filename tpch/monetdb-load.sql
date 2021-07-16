COPY INTO customer FROM '{abs repo}/{abs repo}/raw/customer.tbl' USING DELIMITERS '|';
COPY INTO lineitem FROM '{abs repo}/tpch/raw/lineitem.tbl' USING DELIMITERS '|';
COPY INTO nation FROM '{abs repo}/tpch/raw/nation.tbl' USING DELIMITERS '|';
COPY INTO orders FROM '{abs repo}/tpch/raw/orders.tbl' USING DELIMITERS '|';
COPY INTO part FROM '{abs repo}/tpch/raw/part.tbl' USING DELIMITERS '|';
COPY INTO partsupp FROM '{abs repo}/tpch/raw/partsupp.tbl' USING DELIMITERS '|';
COPY INTO region FROM '{abs repo}/tpch/raw/region.tbl' USING DELIMITERS '|';
COPY INTO supplier FROM '{abs repo}/tpch/raw/supplier.tbl' USING DELIMITERS '|';