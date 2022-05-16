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

def bench(db, query_path):
    cmds = ['.open benchmark/job/duckdb/job.ddb']
    cmds += ['.output /dev/null', 'PRAGMA threads=1;',
             'PRAGMA memory_limit=\'60GB\';', '.timer on']
    queries = glob.glob(query_path + '*.sql')
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
        query = queries[query_idx].replace(query_path, '').replace('.sql', '')
        for time_idx in range(6 * query_idx + 1, 6 * query_idx + 6):
            print('DuckDB', db, query, times[time_idx] * 1000, sep=',')

if __name__ == "__main__":
    bench("JOB (original)", "benchmark/job/queries_robust/original/")
    bench("JOB (increasing)", "benchmark/job/queries_robust/increasing/")
    bench("JOB (decreasing)", "benchmark/job/queries_robust/decreasing/")
    bench("JOB (random)", "benchmark/job/queries_robust/random/")