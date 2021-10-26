import os
import sys

tables = ['region', 'nation', 'part', 'supplier', 'partsupp', 'customer', 'orders', 'lineitem']

def execute(cmd):
    print(cmd, file=sys.stderr)
    os.system(cmd)

def copy(raw, table):
    return 'COPY INTO ' + table + ' FROM \'' +  os.path.abspath(raw + table + ".tbl") + "\' USING DELIMITERS '|';"

if __name__ == "__main__":
    execute('monetdbd create benchmark/tpch10/monetdb')
    execute('monetdbd start benchmark/tpch10/monetdb')

    execute('monetdb create tpch')
    execute('monetdb set nthreads=1 tpch')
    execute('monetdb release tpch')
    execute("mclient -d tpch benchmark/tpch10/schema.sql")
    for table in tables:
        execute("mclient -d tpch -s \"" + copy('benchmark/tpch10/raw/', table) + "\"")
    execute('monetdb stop tpch')


    execute('monetdbd stop benchmark/tpch10/monetdb')