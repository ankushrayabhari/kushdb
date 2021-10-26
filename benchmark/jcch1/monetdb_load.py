import os
import sys

tables = ['region', 'nation', 'part', 'supplier', 'partsupp', 'customer', 'orders', 'lineitem']

def execute(cmd):
    print(cmd, file=sys.stderr)
    os.system(cmd)

def copy(raw, table):
    return 'COPY INTO ' + table + ' FROM \'' +  os.path.abspath(raw + table + ".tbl") + "\' USING DELIMITERS '|';"

if __name__ == "__main__":
    execute('monetdbd create benchmark/jcch1/monetdb')
    execute('monetdbd start benchmark/jcch1/monetdb')

    execute('monetdb create jcch')
    execute('monetdb set nthreads=1 jcch')
    execute('monetdb release jcch')
    execute("mclient -d jcch benchmark/jcch1/schema.sql")
    for table in tables:
        execute("mclient -d jcch -s \"" + copy('benchmark/jcch1/raw/', table) + "\"")
    execute('monetdb stop jcch')


    execute('monetdbd stop benchmark/jcch1/monetdb')