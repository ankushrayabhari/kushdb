import os
import sys

tables = ['region', 'nation', 'part', 'supplier', 'partsupp', 'customer', 'orders', 'lineitem']

def execute(cmd):
    print(cmd, file=sys.stderr)
    os.system(cmd)

def copy(raw, table):
    return 'COPY ' + table + ' FROM \'' +  os.path.abspath(raw + table + ".tbl") + "\' WITH DELIMITER '|';"

def execute_duckdb(cmds):
    with open ('/tmp/duckdb.txt', 'w') as f:
        for cmd in cmds:
            print(cmd, file=sys.stderr)
            print(cmd, file=f)
    execute('duckdb < /tmp/duckdb.txt')

if __name__ == "__main__":
    cmds = ['.open benchmark/tpch/duckdb/tpch1.ddb', '.read benchmark/tpch/schema.sql']
    for table in tables:
        cmds.append(copy('benchmark/tpch/raw-1/', table))
    execute_duckdb(cmds)

    cmds = ['.open benchmark/tpch/duckdb/tpch10.ddb', '.read benchmark/tpch/schema.sql']
    for table in tables:
        cmds.append(copy('benchmark/tpch/raw-10/', table))
    execute_duckdb(cmds)