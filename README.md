# KushDB
Compilation-based execution engine for a database

### Execute TPC-H
1. Run [DBGEN](https://github.com/electrum/tpch-dbgen) to generate .tbl files.
2. Move them to `benchmark/tpch/raw-1/` for scale factor 1 and
   `benchmark/tpch/raw-10` for scale factor 10.
3. Execute: `for i in benchmark/tpch/raw-1/*.tbl benchmark/tpch/raw-10/*.tbl; do sed -i 's/.$//' $i > $i.tight; done`.

#### KushDB
1. Run `bazel build //benchmark/tpch:load`.
2. Run `bazel-bin/benchmark/tpch/load`.
3. To bench, run `python benchmark/tpch/bench.py > tpch.csv`.

#### MonetDB
1. MonetDB must be installed. `monetdbd`, `mclient`, `monetdb` must be available on PATH.
2. Run `python benchmark/tpch/monetdb_load.py`.
3. To bench, run `python benchmark/tpch/monetdb_bench.py > monetdb_tpch.csv`.

#### DuckDB
1. DuckDB must be installed. `duckdb` must be available on PATH.
2. Run `python benchmark/tpch/duckdb_load.py`.
3. To bench, run `python benchmark/tpch/duckdb_load.py > duckdb_tpch.csv`.

#### Hyper
1. Run `bazel build //benchmark/tpch:hyper_load`.
2. Run `bazel-bin/benchmark/tpch/hyper_load`.