# KushDB
Compilation-based execution engine for a database

## Execute TPC-H, JCC-H
0. Instructions will be for SF 1 but same holds for other scale factors.
1. Run dbgen to generate .tbl files.
    - For TPC-H, use [https://github.com/electrum/tpch-dbgen](https://github.com/electrum/tpch-dbgen).
    - For JCC-H, use [https://github.com/ldbc/dbgen.JCC-H](https://github.com/ldbc/dbgen.JCC-H).
2. Move them to `benchmark/tpch1/raw/`.
3. Run `bazel build //benchmark/tpch1:load && bazel-bin/benchmark/tpch1/load`.
4. Run `python benchmark/tpch1/monetdb_load.py`.
5. Run `python benchmark/tpch1/duckdb_load.py`.
6. Run: `duckdb benchmark/tpch1/duckdb/tpch.ddb < benchmark/tpch1/generate_outputs.sql`

#### KushDB
To bench, run `python benchmark/tpch1/bench.py > tpch1.csv`.

#### MonetDB
To bench, run `python benchmark/tpch1/monetdb_bench.py > monetdb_tpch1.csv`.

#### DuckDB
To bench, run `python benchmark/tpch1/duckdb_bench.py > duckdb_tpch1.csv`.

## Execute JOB
1. Download CSV Files from JOB [here](https://github.com/gregrahn/join-order-benchmark).
2. Make the following changes to the CSV files:
- Update line 268268 of `person_info.csv` to escape the `\` in `Canada\US`
- Update line 2671662 of `person_info.csv` to escape the `\` in `(qv); \,`
- Update line 2514451 of `title.csv` to escape the `\` in `\Frag'ile\`
3. Run `python benchmark/job/monetdb_load.py`.
4. Run `python benchmark/job/duckdb_load.py`.

#### MonetDB
To bench, run `python benchmark/job/monetdb_bench.py > monetdb_job.csv`.

#### DuckDB
To bench, run `python benchmark/job/duckdb_bench.py > duckdb_job.csv`.