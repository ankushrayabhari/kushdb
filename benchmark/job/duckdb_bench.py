import os
import sys
import glob

def execute(cmd):
    print(cmd, file=sys.stderr)
    os.system(cmd)

def execute_duckdb(cmds):
    with open ('/tmp/duckdb.txt', 'w') as f:
        for cmd in cmds:
            print(cmd, file=sys.stderr)
            print(cmd, file=f)
    execute('duckdb < /tmp/duckdb.txt > /tmp/bench_time.txt')

def bench(database, use_indexes):
    cmds = ['.open benchmark/job/duckdb/job.ddb']
    if use_indexes:
        cmds.append('.read benchmark/job/indexes.sql')
    cmds += ['.output /dev/null', 'PRAGMA threads=1;',
             'PRAGMA memory_limit=\'60GB\';', '.timer on']
    queries = glob.glob('benchmark/job/queries/*.sql')
    for query in queries:
        for _ in range(6):
            cmds.append('.read ' + query)
    execute_duckdb(cmds)
    times = []
    with open('/tmp/bench_time.txt', 'r') as file:
        data = file.read().replace('\n', ' ')
        for t in data.split():
            try:
                times.append(float(t))
            except ValueError as e:
                continue
    times = times[::3]

    query_idx = 0
    time_idx = 0
    for query_idx in range(len(queries)):
        query = queries[query_idx].replace('benchmark/job/queries/', '').replace('.sql', '')
        for time_idx in range(6 * query_idx + 1, 6 * query_idx + 6):
            print(database, 'JOB', query, times[time_idx] * 1000, sep=',')

if __name__ == "__main__":
    bench('DuckDB', False)
    bench('DuckDB Indexes', True)