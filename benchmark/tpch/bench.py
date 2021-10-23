import os
import sys

def execute(cmd):
    print(cmd, file=sys.stderr)
    os.system(cmd)

def bench(database, benchmark, data_path, use_skinner, flags):
    execute('rm -f benchmark/tpch/data')
    execute('ln -s ' + os.path.abspath(data_path) + ' benchmark/tpch/data')

    queries = ['q02', 'q03', 'q05', 'q07', 'q08', 'q09', 'q10', 'q11',
               'q12', 'q14', 'q18', 'q19']
    for query in queries:
        binary = 'bazel run -c opt --ui_event_filters=-info,-stdout,-stderr --noshow_progress //benchmark/tpch/queries:' + query
        if use_skinner:
            binary += '_skinner'

        binary += ' -- '
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
            for t in times:
                print(database, benchmark, query_num, t, sep=',')

if __name__ == "__main__":
    bench('kushdb ASM (Hash Join)', 'TPC-H SF1', 'benchmark/tpch/data-1', False, ['--backend=asm'])
    bench('kushdb ASM (Skinner Join Permutable)', 'TPC-H SF1', 'benchmark/tpch/data-1', True, ['--backend=asm', '--skinner=permute'])
    bench('kushdb ASM (Skinner Join Recompiling)', 'TPC-H SF1', 'benchmark/tpch/data-1', True, ['--backend=asm', '--skinner=recompile'])

    bench('kushdb ASM (Hash Join)', 'TPC-H SF10', 'benchmark/tpch/data-10', False, ['--backend=asm'])
    bench('kushdb ASM (Skinner Join Permutable)', 'TPC-H SF10', 'benchmark/tpch/data-10', True, ['--backend=asm', '--skinner=permute'])
    bench('kushdb ASM (Skinner Join Recompiling)', 'TPC-H SF10', 'benchmark/tpch/data-10', True, ['--backend=asm', '--skinner=recompile'])

    bench('kushdb LLVM (Hash Join)', 'TPC-H SF1', 'benchmark/tpch/data-1', False, ['--backend=llvm'])
    bench('kushdb LLVM (Skinner Join Permutable)', 'TPC-H SF1', 'benchmark/tpch/data-1', True, ['--backend=llvm', '--skinner=permute'])
    bench('kushdb LLVM (Skinner Join Recompiling)', 'TPC-H SF1', 'benchmark/tpch/data-1', True, ['--backend=llvm', '--skinner=recompile'])

    bench('kushdb LLVM (Hash Join)', 'TPC-H SF10', 'benchmark/tpch/data-10', False, ['--backend=llvm'])
    bench('kushdb LLVM (Skinner Join Permutable)', 'TPC-H SF10', 'benchmark/tpch/data-10', True, ['--backend=llvm', '--skinner=permute'])
    bench('kushdb LLVM (Skinner Join Recompiling)', 'TPC-H SF10', 'benchmark/tpch/data-10', True, ['--backend=llvm', '--skinner=recompile'])