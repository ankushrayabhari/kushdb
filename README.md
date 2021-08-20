# KushDB
Compilation-based execution engine for a database

### Execute TPC-H, Skewed TPC-H
1. Run [DBGEN](https://github.com/electrum/tpch-dbgen) or
[skewed DBGEN](https://www.microsoft.com/en-us/download/details.aspx?id=52430)
to generate .tbl files. For skewed DBGEN, rename `order.tbl` with `orders.tbl`.
2. Move them to `tpch/raw/`.
3. Run: `for i in tpch/raw/*; do sed -i 's/.$//' $i; done`.

#### KushDB
1. Create `tpch/data/`.
2. Run `bazel run //tpch:load` .
3. Execute queries in `tpch/queries/`.

#### MonetDB
1. Create database: `monetdb create tpch`
2. (optional) Set parallelism: `monetdb set nthreads={N} tpch1`
3. Release database: `monetdb release tpch1`
4. Open client: `mclient --timer=performance -d tpch1`. Default user is `monetdb`
   password is `monetdb`.
5. Run `tpch/schema.sql`.
6. Run each line of `tpch/monetdb-load.sql` replacing `{data path}` with the
   data path.
7. Execute queries in `tpch/queries/`. Use `\f trash` to not print output.
