import os
import sys
import glob

def execute(cmd):
    print(cmd, file=sys.stderr)
    os.system(cmd)


if __name__ == "__main__":
    execute('monetdbd start benchmark/job/monetdb')
    execute('monetdb set nthreads=1 job')
    execute('monetdb start job >/dev/null')

    queries = glob.glob('benchmark/job/queries/*.sql')
    # execute once to warm up
    for query in queries:
        cmd = 'mclient --timer=performance -d job ' + query + ' > /dev/null 2> /dev/null'
        execute(cmd)
    # execute 5 times
    for query in queries:
        query = query.replace('benchmark/job/queries/', '').replace('.sql', '')
        cmd = 'mclient --timer=performance -d job benchmark/job/queries/' + query + '.sql > /dev/null 2> /tmp/bench_time.txt'
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
                print('MonetDB', 'JOB', query, times[-1], sep=',')

    execute('monetdb stop job >/dev/null')
    execute('monetdbd stop benchmark/job/monetdb')