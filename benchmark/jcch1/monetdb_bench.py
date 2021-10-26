import os
import sys

def execute(cmd):
    print(cmd, file=sys.stderr)
    os.system(cmd)

queries = ['q02', 'q03', 'q05', 'q07', 'q08', 'q10', 'q11', 'q12', 'q14', 'q18', 'q19']

def bench():
    execute('monetdb set nthreads=1 jcch')
    execute('monetdb start jcch >/dev/null')
    for query in queries:
        query_num = int(query[1:])
        cmd = 'mclient --timer=performance -d jcch benchmark/jcch1/queries/' + query + '.sql > /dev/null 2> /tmp/bench_time.txt'
        # execute once to warm up, 5 times
        execute(cmd)
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