import os
import sys

tables = ['customer', 'lineitem', 'nation', 'orders', 'part', 'partsupp', 'region', 'supplier']

def execute(cmd):
    print(cmd, file=sys.stderr)
    os.system(cmd)

def copy(raw, table):
    return 'COPY INTO ' + table + ' FROM \'' +  os.path.abspath(raw + table + ".tbl") + "\' USING DELIMITERS '|';"

if __name__ == "__main__":
    execute('monetdbd create benchmark/tpch/monetdb')
    execute('monetdbd start benchmark/tpch/monetdb')

    execute('monetdb create tpch-1')
    execute('monetdb set nthreads=1 tpch-1')
    execute('monetdb release tpch-1')
    execute("mclient -d tpch-1 benchmark/tpch/schema.sql")
    for table in tables:
        execute("mclient -d tpch-1 -s \"" + copy('benchmark/tpch/raw-1/', table) + "\"")
    execute('monetdb stop tpch-1')

    execute('monetdb create tpch-10')
    execute('monetdb set nthreads=1 tpch-10')
    execute('monetdb release tpch-10')
    execute("mclient -d tpch-10 benchmark/tpch/schema.sql")
    for table in tables:
        execute("mclient -d tpch-10 -s \"" + copy('benchmark/tpch/raw-10/', table) + "\"")
    execute('monetdb stop tpch-10')

    execute('monetdbd stop benchmark/tpch/monetdb')