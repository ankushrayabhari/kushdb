import os
import sys

tables = ['region', 'nation', 'part', 'supplier', 'partsupp', 'customer', 'orders', 'lineitem']

def execute(cmd):
    print(cmd, file=sys.stderr)
    os.system(cmd)

def copy(raw, table):
    return 'COPY INTO ' + table + ' FROM \'' +  os.path.abspath(raw + table + ".tbl") + "\' USING DELIMITERS '|';"

if __name__ == "__main__":
    execute('monetdbd create benchmark/tpch1/monetdb')
    execute('monetdbd start benchmark/tpch1/monetdb')

    execute('monetdb create tpch')
    execute('monetdb set nthreads=1 tpch')
    execute('monetdb release tpch')
    execute("mclient -d tpch benchmark/tpch1/schema.sql")
    for table in tables:
        execute("mclient -d tpch -s \"" + copy('benchmark/tpch1/raw/', table) + "\"")
    execute('monetdb stop tpch')


    execute('monetdbd stop benchmark/tpch1/monetdb')