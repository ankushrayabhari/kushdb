# KushDB
Compilation-based execution engine for a database

### Execute TPC-H
1. Run [DBGEN](https://github.com/electrum/tpch-dbgen) to generate .tbl files.
2. Move them to `benchmark/tpch/raw-1/` for scale factor 1 and
   `benchmark/tpch/raw-10` for scale factor 10.
3. Execute: `for i in benchmark/tpch/raw-1/*.tbl benchmark/tpch/raw-10/*.tbl; do sed 's/.$//' $i > $i.tight; done`.

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

### Execute JOB
1. Download CSV Files from JOB [here](https://github.com/gregrahn/join-order-benchmark).
2. Make the following changes to the CSV files:
- Update line 268268 of person_info.csv to escape the \ in "Canada\US"
- Update line 2671662 of person_info.csv to escape the \ in "(qv); \,"
- Update line 2514451 of title.csv to escape the \ in "\Frag'ile\"

#### MonetDB
1. MonetDB must be installed. `monetdbd`, `mclient`, `monetdb` must be available on PATH.
2. Run `python benchmark/job/monetdb_load.py`.
3. To bench, run `python benchmark/job/monetdb_bench.py > monetdb_job.csv`.
