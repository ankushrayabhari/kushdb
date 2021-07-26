COPY INTO customer FROM '{data path}/customer.tbl' USING DELIMITERS '|';
COPY INTO lineitem FROM '{data path}/lineitem.tbl' USING DELIMITERS '|';
COPY INTO nation FROM '{data path}/nation.tbl' USING DELIMITERS '|';
COPY INTO orders FROM '{data path}/orders.tbl' USING DELIMITERS '|';
COPY INTO part FROM '{data path}/part.tbl' USING DELIMITERS '|';
COPY INTO partsupp FROM '{data path}/partsupp.tbl' USING DELIMITERS '|';
COPY INTO region FROM '{data path}/region.tbl' USING DELIMITERS '|';
COPY INTO supplier FROM '{data path}/supplier.tbl' USING DELIMITERS '|';