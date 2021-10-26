# KushDB
Compilation-based execution engine for a database

### Execute TPC-H
1. Run [DBGEN](https://github.com/electrum/tpch-dbgen) to generate .tbl files.
2. Move them to `benchmark/tpch1/raw/` for scale factor 1.
3. Execute: `for i in benchmark/tpch/raw1/*.tbl; do sed 's/.$//' $i > $i.tight; done`.

#### KushDB
1. Run `bazel build //benchmark/tpch1:load`.
2. Run `bazel-bin/benchmark/tpch1/load`.
3. To bench, run `python benchmark/tpch1/bench.py > tpch1.csv`.

#### MonetDB
1. MonetDB must be installed. `monetdbd`, `mclient`, `monetdb` must be available on PATH.
2. Run `python benchmark/tpch1/monetdb_load.py`.
3. To bench, run `python benchmark/tpch1/monetdb_bench.py > monetdb_tpch1.csv`.

#### DuckDB
1. DuckDB must be installed. `duckdb` must be available on PATH.
2. Run `python benchmark/tpch1/duckdb_load.py`.
3. To bench, run `python benchmark/tpch1/duckdb_bench.py > duckdb_tpch1.csv`.

### Execute JOB
1. Download CSV Files from JOB [here](https://github.com/gregrahn/join-order-benchmark).
2. Make the following changes to the CSV files:
- Update line 268268 of `person_info.csv` to escape the `\` in `Canada\US`
- Update line 2671662 of `person_info.csv` to escape the `\` in `(qv); \,`
- Update line 2514451 of `title.csv` to escape the `\` in `\Frag'ile\`

#### MonetDB
1. MonetDB must be installed. `monetdbd`, `mclient`, `monetdb` must be available on PATH.
2. Run `python benchmark/job/monetdb_load.py`.
3. To bench, run `python benchmark/job/monetdb_bench.py > monetdb_job.csv`.

#### DuckDB
1. DuckDB must be installed. `duckdb` must be available on PATH.
2. Run `python benchmark/job/duckdb_load.py`.
3. To bench, run `python benchmark/job/duckdb_bench.py > duckdb_job.csv`.