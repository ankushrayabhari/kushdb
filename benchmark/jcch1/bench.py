import os
import sys

def execute(cmd):
    print(cmd, file=sys.stderr)
    os.system(cmd)

def bench(database, flags):
    queries = ['q01', 'q02', 'q03', 'q05', 'q06', 'q07', 'q08', 'q09', 'q10',
               'q11', 'q12', 'q14', 'q18', 'q19']
    for query in queries:
        binary = 'bazel run -c opt --ui_event_filters=-info,-stdout,-stderr --noshow_progress //benchmark/jcch1/queries:' + query

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
                print(database, 'JCC-H SF1', query_num, t, sep=',')

if __name__ == "__main__":
    bench('kushdb ASM (Skinner Join Permutable)', ['--backend=asm', '--skinner_join=permute'])
    bench('kushdb ASM (Skinner Join Recompiling)', ['--backend=asm', '--skinner_join=recompile'])
    bench('kushdb LLVM (Skinner Join Permutable)', ['--backend=llvm', '--skinner_join=permute'])
    bench('kushdb LLVM (Skinner Join Recompiling)', ['--backend=llvm', '--skinner_join=recompile'])