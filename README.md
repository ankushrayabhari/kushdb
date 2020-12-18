DBGEN to generate .tbl files:
Run on each table: sed -i 's/.$//' file.tbl

SET max_parallel_workers TO 1;
SET jit TO off;