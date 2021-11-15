import os
import sys

def execute(cmd):
    print(cmd, file=sys.stderr)
    os.system(cmd)

def execute_duckdb(cmds):
    with open ('/tmp/duckdb.txt', 'w') as f:
        for cmd in cmds:
            print(cmd, file=sys.stderr)
            print(cmd, file=f)
    execute('duckdb < /tmp/duckdb.txt > /tmp/bench_time.txt')

queries = ['q01', 'q02', 'q03', 'q05', 'q06', 'q07', 'q08', 'q09', 'q10', 'q11', 'q12', 'q14', 'q18', 'q19']

def bench(db, use_indexes):
    cmd_base = ['.open benchmark/tpch1/duckdb/tpch.ddb']
    if use_indexes:
        cmd_base.append('.read benchmark/tpch1/indexes.sql')
    cmd_base += ['.output /dev/null', 'PRAGMA threads=1;',
                'PRAGMA memory_limit=\'60GB\';', '.timer on']
    for query in queries:
        query_num = int(query[1:])
        cmds = cmd_base.copy()
        for _ in range(6):
            cmds.append('.read benchmark/tpch1/queries/' + query + '.sql')
        execute_duckdb(cmds)
        with open('/tmp/bench_time.txt', 'r') as file:
            data = file.read().replace('\n', ' ')
            times = []
            for t in data.split():
                try:
                    times.append(float(t))
                except ValueError as e:
                    continue
            for t in times[3::3]:
                print(db, 'TPC-H SF1', query_num, t * 1000, sep=',')

if __name__ == "__main__":
    bench('DuckDB', False)
    bench('DuckDB (Indexes)', True)