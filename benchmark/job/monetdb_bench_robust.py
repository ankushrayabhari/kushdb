import os
import sys
import glob

def execute(cmd):
    print(cmd, file=sys.stderr)
    os.system(cmd)

def bench(db, query_path):
    execute('monetdbd start benchmark/job/monetdb')
    execute('monetdb set nthreads=1 job')
    execute('monetdb start job >/dev/null')

    queries = glob.glob(query_path + '*.sql')
    # execute once to warm up
    for query in queries:
        cmd = 'mclient --timer=performance -d job ' + query + ' > /dev/null 2> /dev/null'
        execute(cmd)
    # execute 5 times
    for query in queries:
        query = query.replace(query_path, '').replace('.sql', '')
        cmd = 'mclient --timer=performance -d job ' + query_path + query + '.sql > /dev/null 2> /tmp/bench_time.txt'
        for _ in range(5):
            execute(cmd)
            with open('/tmp/bench_time.txt', 'r') as file:
                data = file.read().replace(':', ' ').replace('\n', ' ')
                times = []
                for t in data.split():
                    try:
                        times.append(float(t))
                    except ValueError as e:
                        continue
                print('MonetDB', db, query, times[-1], sep=',')

    execute('monetdb stop job >/dev/null')
    execute('monetdbd stop benchmark/job/monetdb')


if __name__ == "__main__":
    bench("JOB (original)", "benchmark/job/queries_robust/original/")
    bench("JOB (increasing)", "benchmark/job/queries_robust/increasing/")
    bench("JOB (decreasing)", "benchmark/job/queries_robust/decreasing/")
    bench("JOB (random)", "benchmark/job/queries_robust/random/")