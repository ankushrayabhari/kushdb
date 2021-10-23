import os
import sys

def execute(cmd):
    print(cmd, file=sys.stderr)
    os.system(cmd)

queries = ['q02', 'q03', 'q05', 'q07', 'q08', 'q09', 'q10', 'q11', 'q12', 'q14', 'q18', 'q19']

def bench(benchmark, db):
    execute('monetdb set nthreads=1 ' + db)
    execute('monetdb start ' + db + ' >/dev/null')
    for query in queries:
        query_num = int(query[1:])
        cmd = 'mclient --timer=performance -d ' + db + ' benchmark/tpch/queries/' + query + '.sql > /dev/null 2> /tmp/bench_time.txt'
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
                print('MonetDB', benchmark, query_num, times[-1], sep=',')
    execute('monetdb stop ' + db + ' >/dev/null')

if __name__ == "__main__":
    execute('monetdbd start benchmark/tpch/monetdb')

    bench('TPC-H SF1', 'tpch1')
    bench('TPC-H SF10', 'tpch10')

    execute('monetdbd stop benchmark/tpch/monetdb')