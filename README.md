# KushDB
Compilation-based execution engine for a database

### Setup TPC-H
1. Run DBGEN to generate .tbl files.
2. Move them to `tpch/raw/`.
3. Run: `for i in tpch/raw/*; do sed -i 's/.$//' $i; done`.
4. Create `tpch/data/`.
5. Run `bazel build //tpch:load && bazel-bin/tpch/load` .
6. Execute queries in `tpch/queries/`.