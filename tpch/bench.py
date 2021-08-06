import os
import sys

def execute(cmd):
    print(cmd, file=sys.stderr)
    os.system(cmd)

def bench(database, benchmark, data_path, use_skinner, flags):
    execute('rm -f tpch/data')
    execute('ln -s ' + os.path.abspath(data_path) + ' tpch/data')

    queries = ['q02', 'q03', 'q04', 'q05', 'q07', 'q08', 'q09', 'q10', 'q11',
               'q12', 'q14', 'q18', 'q19']
    for query in queries:
        binary = './bazel-bin/tpch/queries/' + query
        if use_skinner:
            binary += '_skinner'
        for f in flags:
            binary += ' ' + f
        redirect = ' > /dev/null 2> /tmp/bench_time.txt'
        executable = binary + redirect

        execute(executable)

        query_num = int(query[1:])

        with open('/tmp/bench_time.txt', 'r') as file:
            data = file.read().replace('\n', ' ')
            times = []
            for t in data.split():
                try:
                    times.append(float(t))
                except ValueError as e:
                    continue
            print(database, benchmark, query_num, times[0], times[1], sep=',')

if __name__ == "__main__":
    bench('kushdb (Hash Join)', 'TPC-H SF1', 'tpch/data-1', False, [])
    bench('kushdb (Skinner Join Permutable)', 'TPC-H SF1', 'tpch/data-1', True, [])
    bench('kushdb (Hash Join)', 'TPC-H Skewed SF1', 'tpch/data-skewed-1', False, [])
    bench('kushdb (Skinner Join Permutable)', 'TPC-H Skewed SF1', 'tpch/data-skewed-1', True, [])
    bench('kushdb (Hash Join)', 'TPC-H SF10', 'tpch/data-10', False, [])
    bench('kushdb (Skinner Join Permutable)', 'TPC-H SF10', 'tpch/data-10', True, [])
    bench('kushdb (Hash Join)', 'TPC-H Skewed SF10', 'tpch/data-skewed-10', False, [])
    bench('kushdb (Skinner Join Permutable)', 'TPC-H Skewed SF10', 'tpch/data-skewed-10', True, [])