import os
import sys

def execute(cmd):
    print(cmd, file=sys.stderr)
    os.system(cmd)

queries = ['q01', 'q02', 'q03', 'q05', 'q06', 'q07', 'q08', 'q09', 'q10', 'q11', 'q12', 'q14', 'q18', 'q19']

def bench():
    execute('monetdb set nthreads=1 jcch')
    execute('monetdb start jcch >/dev/null')
    # execute once to warm up
    for query in queries:
        cmd = 'mclient --timer=performance -d jcch benchmark/jcch1/queries/' + query + '.sql > /dev/null 2> /dev/null'
        execute(cmd)
    # execute 5 times
    for query in queries:
        query_num = int(query[1:])
        cmd = 'mclient --timer=performance -d jcch benchmark/jcch1/queries/' + query + '.sql > /dev/null 2> /tmp/bench_time.txt'
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
                print('MonetDB', 'JCC-H SF1', query_num, times[-1], sep=',')
    execute('monetdb stop jcch >/dev/null')

if __name__ == "__main__":
    execute('monetdbd start benchmark/jcch1/monetdb')
    bench()
    execute('monetdbd stop benchmark/jcch1/monetdb')