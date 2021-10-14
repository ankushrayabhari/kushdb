import os
import sys

tables = ['region', 'nation', 'part', 'supplier', 'partsupp', 'customer', 'orders', 'lineitem']

def execute(cmd):
    print(cmd, file=sys.stderr)
    os.system(cmd)

def copy(raw, table):
    return 'COPY INTO ' + table + ' FROM \'' +  os.path.abspath(raw + table + ".tbl") + "\' USING DELIMITERS '|';"

if __name__ == "__main__":
    execute('monetdbd create benchmark/tpch/monetdb')
    execute('monetdbd start benchmark/tpch/monetdb')

    execute('monetdb create tpch1')
    execute('monetdb set nthreads=1 tpch1')
    execute('monetdb release tpch1')
    execute("mclient -d tpch1 benchmark/tpch/schema.sql")
    for table in tables:
        execute("mclient -d tpch1 -s \"" + copy('benchmark/tpch/raw-1/', table) + "\"")
    execute('monetdb stop tpch1')

    execute('monetdb create tpch10')
    execute('monetdb set nthreads=1 tpch10')
    execute('monetdb release tpch10')
    execute("mclient -d tpch10 benchmark/tpch/schema.sql")
    for table in tables:
        execute("mclient -d tpch10 -s \"" + copy('benchmark/tpch/raw-10/', table) + "\"")
    execute('monetdb stop tpch10')

    execute('monetdbd stop benchmark/tpch/monetdb')