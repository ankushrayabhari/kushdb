# KushDB
Compilation-based execution engine for a database

### Setup TPC-H
1. Run DBGEN to generate .tbl files.
2. Run on each table file: sed -i 's/.$//' file.tbl
3. Move them to `tpch/raw/`.
4. Run `bazel build //tpch:load && bazel-bin/tpch/load` 
5. Execute queries in `tpch/queries/`

### Setup TPC-H on Postgres

1. Run DBGEN to generate .tbl files.
2. Run on each table file: sed -i 's/.$//' file.tbl
3. Move them to `tpch/raw/`.
4. Run `tpch/schema.sql`.
5. Run `tpch/postgres-load.sql`.