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

if __name__ == "__main__":
    cmd_base = ['.open benchmark/job/duckdb/job.ddb',
                '.output /dev/null', 'PRAGMA threads=1;',
                'PRAGMA memory_limit=\'60GB\';', '.timer on']
    queries = glob.glob('benchmark/job/queries/*.sql')
    for query in queries:
        query = query.replace('benchmark/job/queries/', '').replace('.sql', '')
        cmds = cmd_base.copy()
        for _ in range(6):
            cmds.append('.read benchmark/job/queries/' + query + '.sql')
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
                print('DuckDB', 'JOB', query, t * 1000, sep=',')