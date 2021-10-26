import os
import sys

queries = ['q01', 'q02', 'q03', 'q05', 'q06', 'q07', 'q08', 'q09', 'q10', 'q11', 'q12', 'q14', 'q18', 'q19']

def execute(cmd):
    print(cmd, file=sys.stderr)
    os.system(cmd)

def generate(query):
    return [
        '.output benchmark/tpch1/raw/' + query + '.tbl',
        '.read benchmark/tpch1/queries/' + query + '.sql',
    ]

def execute_duckdb(cmds):
    with open ('/tmp/duckdb.txt', 'w') as f:
        for cmd in cmds:
            print(cmd, file=sys.stderr)
            print(cmd, file=f)
    execute('duckdb benchmark/tpch1/duckdb/tpch.ddb < /tmp/duckdb.txt')

def clean(query):
    execute("sed -i 's/\\r$//' benchmark/tpch1/raw/" + query + '.tbl')

if __name__ == "__main__":
    cmds = ['.mode csv', ".separator '|'", '.headers off']
    for query in queries:
        cmds.extend(generate(query))
    execute_duckdb(cmds)
    for query in queries:
        clean(query)
