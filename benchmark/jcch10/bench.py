import os
import sys


def execute(cmd):
    print(cmd, file=sys.stderr)
    os.system(cmd)


def bench(database, flags):
    queries = ['q01', 'q02', 'q03', 'q05', 'q06', 'q07', 'q08', 'q09', 'q10',
               'q11', 'q12', 'q14', 'q18', 'q19']
    for query in queries:
        binary = 'bazel run -c opt --ui_event_filters=-info,-stdout,-stderr --noshow_progress //benchmark/jcch10/queries:' + query

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
                print(database, 'JCC-H SF10', query_num, t, sep=',')


if __name__ == "__main__":
    bench('kushdb (ASM Permute)',       ['--use_dictionary=false', '--skinner_join=permute',   '--backend=asm'])
    bench('kushdb (ASM Recompile)',     ['--use_dictionary=false', '--skinner_join=recompile', '--backend=asm'])
    bench('kushdb (LLVM Permute)',      ['--use_dictionary=false', '--skinner_join=permute',   '--backend=llvm'])
    bench('kushdb (LLVM Recompile)',    ['--use_dictionary=false', '--skinner_join=recompile', '--backend=llvm'])
    bench('kushdb (Hybrid)',            ['--use_dictionary=false', '--skinner_join=hybrid'])
    bench('kushdb DC (ASM Permute)',    ['--use_dictionary=true',  '--skinner_join=permute',   '--backend=asm'])
    bench('kushdb DC (ASM Recompile)',  ['--use_dictionary=true',  '--skinner_join=recompile', '--backend=asm'])
    bench('kushdb DC (LLVM Permute)',   ['--use_dictionary=true',  '--skinner_join=permute',   '--backend=llvm'])
    bench('kushdb DC (LLVM Recompile)', ['--use_dictionary=true',  '--skinner_join=recompile', '--backend=llvm'])
    bench('kushdb DC (Hybrid)',         ['--use_dictionary=true',  '--skinner_join=hybrid'])
